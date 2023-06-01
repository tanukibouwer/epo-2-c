/* nuklear - v1.32.0 - public domain */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../thirdparty/glfw/include/GLFW/glfw3.h"

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_GLFW_GL2_IMPLEMENTATION
#define NK_KEYSTATE_BASED_INPUT
#define NK_INCLUDE_FIXED_TYPES

#include "../thirdparty/nuklear.h"
#include "../thirdparty/nuklear_glfw_gl2.h"

#include "grid.h"
#include "gui_helpers.h"
#include "../backend/algorithms/astar.h"

#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 800

enum Challenges {
    CHALLENGE_A,
    CHALLENGE_B,
    CHALLENGE_C
};

enum Challenges current_challenge = CHALLENGE_A;
bool challenge_in_progress = false;
struct Path current_path;
int last_selected_point = 0;
struct Point start_point;
struct Point end_point;
struct PointConnection test_mines[50];
int mine_count = 0;
int last_selected_mine_point = 0;
struct Point mine_point1;
struct Point mine_point2;
struct PointConnection robot_position;
int last_selected_robot_point = 0;

static void error_callback(int e, const char *d) { printf("Error %d: %s\n", e, d); }

int main(void) {
    /* Platform */
    static GLFWwindow *win;
    int width = 0, height = 0;

    /* GUI */
    struct nk_context *ctx;
    struct nk_colorf bg;

    /* GLFW */
    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) {
        fprintf(stdout, "[GFLW] failed to init!\n");
        exit(1);
    }
    win = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "EPO-2", NULL, NULL);
    glfwMakeContextCurrent(win);
    glfwGetWindowSize(win, &width, &height);

    /* GUI */
    ctx = nk_glfw3_init(win, NK_GLFW3_INSTALL_CALLBACKS);
    /* Load Fonts: if none of these are loaded a default font will be used  */
    /* Load Cursor: if you uncomment cursor loading please hide the cursor */
    {
        struct nk_font_atlas *atlas;
        nk_glfw3_font_stash_begin(&atlas);
        struct nk_font *franklin = nk_font_atlas_add_from_file(atlas, "libre_franklin.ttf", 18, 0);
        nk_glfw3_font_stash_end();
        nk_style_set_font(ctx, &franklin->handle);
    }

    bg.r = 0.10f, bg.g = 0.18f, bg.b = 0.24f, bg.a = 1.0f;

    while (!glfwWindowShouldClose(win)) {
        // Input
        glfwPollEvents();
        nk_glfw3_new_frame();

        bool grid_selected = false;
        struct Point grid_selection;

        // GUI
        if (nk_begin(ctx, "EPO-2", nk_rect(0, 0, (float) width, (float) height),
                     NK_WINDOW_TITLE | NK_WINDOW_CLOSABLE
        ))
        {
            if (!challenge_in_progress) {
                vertical_spacer(ctx, 10);
                nk_layout_row_dynamic(ctx, 30, 1);
                nk_label(ctx, "Challenge Select", NK_TEXT_ALIGN_CENTERED);

                nk_layout_row_dynamic(ctx, 120, 3);

                if (nk_group_begin(ctx, "Challenge Selection", NK_WINDOW_NO_SCROLLBAR)) {
                    nk_layout_row_dynamic(ctx, 105, 2);
                    nk_spacing(ctx, 1);
                    if (nk_group_begin(ctx, "Challenges", NK_WINDOW_NO_SCROLLBAR)) {
                        nk_layout_row_dynamic(ctx, 30, 1);
                        current_challenge = nk_option_label(ctx, "Challenge A", current_challenge == CHALLENGE_A) ? CHALLENGE_A : current_challenge;
                        current_challenge = nk_option_label(ctx, "Challenge B", current_challenge == CHALLENGE_B) ? CHALLENGE_B : current_challenge;
                        current_challenge = nk_option_label(ctx, "Challenge C", current_challenge == CHALLENGE_C) ? CHALLENGE_C : current_challenge;
                        nk_group_end(ctx);
                    }
                    nk_group_end(ctx);
                }

                if (nk_button_label(ctx, "START CHALLENGE")) {
                    challenge_in_progress = true;
                }

                if (nk_group_begin(ctx, "Robot Status:", NK_WINDOW_TITLE)) {
                    nk_layout_row_dynamic(ctx, 30, 1);
                    nk_label_colored(ctx, "No signal.", NK_TEXT_ALIGN_LEFT, nk_red);
                    nk_label_colored(ctx, "Verdict: NOT READY.", NK_TEXT_ALIGN_LEFT, nk_red);
                    nk_group_end(ctx);
                }
            }
            else {
                vertical_spacer(ctx, 10);
                nk_layout_row_dynamic(ctx, 120, 2);
                char text[12] = "Challenge A:";
                strcpy(text, current_challenge == CHALLENGE_B ? "Challenge B:" : (current_challenge == CHALLENGE_C ? "Challenge C:" : text));
                if (nk_group_begin(ctx, text, NK_WINDOW_TITLE)) {
                    nk_layout_row_dynamic(ctx, 30, 1);
                    nk_label_colored(ctx, "Going from S1 to S12.", NK_TEXT_ALIGN_LEFT, nk_white);
                    nk_label_colored(ctx, "No mines detected thus far.", NK_TEXT_ALIGN_LEFT, nk_white);
                    nk_group_end(ctx);
                }
                if (nk_group_begin(ctx, "Robot Status:", NK_WINDOW_TITLE)) {
                    nk_layout_row_dynamic(ctx, 30, 1);
                    nk_label_colored(ctx, "Challenge in progress...", NK_TEXT_ALIGN_LEFT, nk_green);
                    nk_label_colored(ctx, "Waiting for robot to reach crossing...", NK_TEXT_ALIGN_LEFT, nk_white);
                    nk_group_end(ctx);
                }
            }

            vertical_spacer(ctx, 10);

            int grid_width = 600;
            int grid_height = 620;
            float ratio[3];
            ratio[0] = ratio[2] = (float)(width - grid_width) / 2;
            ratio[1] = (float)grid_width;
            nk_layout_row(ctx, NK_STATIC, (float)grid_height, 3, ratio);
            nk_spacing(ctx, 1);
            if (nk_group_begin(ctx, "grid", 0)) {
                grid_selection = draw_grid(ctx, current_path, test_mines, mine_count, &grid_selected, robot_position);
                nk_group_end(ctx);
            }

        }
        nk_end(ctx);

        if (nk_window_is_hidden(ctx, "EPO-2")) {
            glfwSetWindowShouldClose(win, GLFW_TRUE);
        }

        // Handle GUI input.
        if (grid_selected) {
            // Path point
            if (glfwGetKey(win, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {

                // Mine point.
                if (last_selected_mine_point == 0) {
                    mine_point2 = grid_selection;
                    last_selected_mine_point = 1;
                }
                else {
                    mine_point1 = grid_selection;
                    last_selected_mine_point = 0;
                }

                if (are_points_adjacent(mine_point1, mine_point2)) {
                    struct PointConnection connection;
                    connection.point1 = mine_point1;
                    connection.point2 = mine_point2;

                    bool already_exists = false;
                    for (int i = 0; i < mine_count; i++) {
                        if (is_point_connection_equal(connection, test_mines[i])) {
                            already_exists = true;
                        }
                    }

                    if (already_exists) {
                        struct PointConnection new_mines[50];
                        int new_count = 0;
                        for (int i = 0; i < mine_count; i++) {
                            if (!is_point_connection_equal(connection, test_mines[i])) {
                                new_mines[new_count++] = test_mines[i];
                            }
                        }

                        for (int i = 0; i < new_count; i++) {
                            test_mines[i] = new_mines[i];
                        }
                        mine_count = new_count;

                    }
                    else {
                        test_mines[mine_count] = connection;
                        mine_count++;
                    }
                }
                current_path = get_path(start_point, end_point, test_mines, mine_count);
            }
            else if (glfwGetKey(win, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
                if (last_selected_robot_point == 0) {
                    robot_position.point1 = grid_selection;
                    last_selected_robot_point = 1;
                } else {
                    robot_position.point2 = grid_selection;
                    last_selected_robot_point = 0;
                }
            }
            else {
                if (last_selected_point == 0) {
                    end_point = grid_selection;
                    last_selected_point = 1;
                } else {
                    start_point = grid_selection;
                    last_selected_point = 0;
                }

                current_path = get_path(start_point, end_point, test_mines, mine_count);
            }
        }

        // Draw
        glfwGetWindowSize(win, &width, &height);
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT);
        glClearColor(bg.r, bg.g, bg.b, bg.a);
        nk_glfw3_render(NK_ANTI_ALIASING_ON);
        glfwSwapBuffers(win);
    }

    nk_glfw3_shutdown();
    glfwTerminate();
    return 0;
}