#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include "raylib.h"

typedef struct Log Log;
struct Log {
    float perf;
    float score;
    float episode_return;
    float episode_length;
    float n;
};

typedef struct Client Client;
typedef struct Player Player;
typedef struct SpeedHockey SpeedHockey;

struct Player {
    float behind_paddle_y;
    float front_paddle_y;
    float behind_paddle_x;
    float front_paddle_x;
    float behind_paddle_dir;
    float front_paddle_dir;
};

#define PLAYER_COUNT 2

struct SpeedHockey {
    Client* client;
    Log log;
    float* observations;
    float* actions;
    float* rewards;
    unsigned char* terminals;
    Player players[PLAYER_COUNT];
    unsigned int score_p1;
    unsigned int score_p2;
    float goal_offset;
    float paddle_speed;
    float paddle_y_offset;
    float paddle_x_front_offset;
    float paddle_x_behind_offset;
    float min_paddle_y;
    float max_paddle_y;
    float ball_x;
    float ball_y;
    float ball_vx;
    float ball_vy;
    float width;
    float height;
    float paddle_width;
    float paddle_height;
    float ball_width;
    float ball_height;
    float ball_initial_speed_x;
    float ball_initial_speed_y;
    float ball_max_speed_y;
    float ball_speed_y_increment;
    unsigned int max_score;
    int tick;
    int n_bounces;
    int win;
    int frameskip;
    int continuous;
};

void init(SpeedHockey* env) {
    // logging
    env->tick = 0;
    env->n_bounces = 0;
    env->win = 0;

    // precompute
    env->min_paddle_y = env->paddle_height / 2;
    env->max_paddle_y = env->height - env->paddle_height;
    
    env->players[0].behind_paddle_x = env->paddle_x_behind_offset;
    env->players[0].front_paddle_x = env->paddle_x_front_offset;
    env->players[1].behind_paddle_x = env->width - env->paddle_x_behind_offset - env->paddle_width;
    env->players[1].front_paddle_x = env->width - env->paddle_x_front_offset - env->paddle_width;

    for(int i = 0; i < PLAYER_COUNT; i++) {
        env->players[i].behind_paddle_dir = 0;
        env->players[i].front_paddle_dir = 0;
    }
}

void allocate(SpeedHockey* env) {
    init(env);
    env->observations = (float*)calloc(8*PLAYER_COUNT, sizeof(float));
    env->actions = (float*)calloc(7*PLAYER_COUNT, sizeof(float));
    env->rewards = (float*)calloc(PLAYER_COUNT, sizeof(float));
    env->terminals = (unsigned char*)calloc(PLAYER_COUNT, sizeof(unsigned char));
}

void free_allocated(SpeedHockey* env) {
    free(env->observations);
    free(env->actions);
    free(env->rewards);
    free(env->terminals);
}

void c_close(SpeedHockey* env) {
}

void add_log(SpeedHockey* env) {
    float score = (float)env->score_p1 - (float)env->score_p2;
    env->log.episode_length += env->tick;
    env->log.episode_return += score;
    env->log.score += score;
    env->log.perf += (float)(env->score_p1) / ((float)env->score_p2 + (float)env->score_p1);
    env->log.n += 1;
}

void compute_observations(SpeedHockey* env) {
    int obs_index = 0;
    for(int i = 0; i < PLAYER_COUNT; i++) {
        // env->observations[obs_index++] = i;
        if(i == 0) {
            env->observations[obs_index++] = (env->players[i].behind_paddle_y - env->min_paddle_y) / (env->max_paddle_y - env->min_paddle_y);
            env->observations[obs_index++] = (env->players[i].front_paddle_y - env->min_paddle_y) / (env->max_paddle_y - env->min_paddle_y);
            env->observations[obs_index++] = (env->players[1-i].behind_paddle_y - env->min_paddle_y) / (env->max_paddle_y - env->min_paddle_y);
            env->observations[obs_index++] = (env->players[1-i].front_paddle_y - env->min_paddle_y) / (env->max_paddle_y - env->min_paddle_y);
            env->observations[obs_index++] = env->ball_x / env->width;
            env->observations[obs_index++] = env->ball_y / env->height;
            env->observations[obs_index++] = (env->ball_vx + env->ball_initial_speed_x) / (2 * env->ball_initial_speed_x);
            env->observations[obs_index++] = (env->ball_vy + env->ball_max_speed_y) / (2 * env->ball_max_speed_y);
        } else {
            env->observations[obs_index++] = ((env->height - env->players[i].behind_paddle_y)- env->min_paddle_y) / (env->max_paddle_y - env->min_paddle_y);
            env->observations[obs_index++] = ((env->height - env->players[i].front_paddle_y) - env->min_paddle_y) / (env->max_paddle_y - env->min_paddle_y);
            env->observations[obs_index++] = ((env->height - env->players[1-i].behind_paddle_y) - env->min_paddle_y) / (env->max_paddle_y - env->min_paddle_y);
            env->observations[obs_index++] = ((env->height - env->players[1-i].front_paddle_y) - env->min_paddle_y) / (env->max_paddle_y - env->min_paddle_y);
            env->observations[obs_index++] = (env->width - env->ball_x) / env->width;
            env->observations[obs_index++] = (env->height - env->ball_y) / env->height;
            env->observations[obs_index++] = (-env->ball_vx + env->ball_initial_speed_x) / (2 * env->ball_initial_speed_x);
            env->observations[obs_index++] = (-env->ball_vy + env->ball_max_speed_y) / (2 * env->ball_max_speed_y);
        }
        // env->observations[obs_index++] = env->score_p1 / env->max_score;
        // env->observations[obs_index++] = env->score_p2 / env->max_score;
    }
}

void reset_round(SpeedHockey* env) {
    for(int i = 0; i < PLAYER_COUNT; i++) {
        env->players[i].behind_paddle_y = env->height / 2 - env->paddle_height / 2;
        env->players[i].front_paddle_y = env->height / 2 - env->paddle_height / 2;
    }

    env->ball_x = env->width / 2 - env->ball_width / 2;;
    env->ball_y = env->height / 2 - env->ball_height / 2;
    env->ball_vx = (2*(rand() % 2) - 1) * env->ball_initial_speed_x;
    env->ball_vy = (2*(rand() % 2) - 1) * env->ball_initial_speed_y;
    env->tick = 0;
    env->n_bounces = 0;
}

void c_reset(SpeedHockey* env) {
    reset_round(env);
    env->score_p1 = 0;
    env->score_p2 = 0;
    compute_observations(env);
}

struct Delta {
    float dx;
    float dy;
};

// struct CollisionResult {

// }

bool check_collision_behind(SpeedHockey* env, int player_index) {    
    float start_x = env->players[player_index].behind_paddle_x;
    float end_x = env->players[player_index].behind_paddle_x + env->paddle_width;
    float start_y = env->players[player_index].behind_paddle_y;
    float end_y = env->players[player_index].behind_paddle_y + env->paddle_height;
    // if(player_index == 0) {
    //     start_x = env->paddle_x_behind_offset;
    //     end_x = env->paddle_x_behind_offset + env->paddle_width;
    // } else {
    //     start_x = env->width - env->paddle_x_behind_offset;
    //     end_x = env->width - env->paddle_x_behind_offset + env->paddle_width;
    // }
    float ball_start_x = env->ball_x;
    float ball_end_x = env->ball_x + env->ball_width;
    float ball_start_y = env->ball_y;
    float ball_end_y = env->ball_y + env->ball_height;

    int player_mult = (player_index * 2) - 1;

    if(start_x <= ball_end_x && ball_start_x <= end_x &&
        start_y <= ball_end_y && ball_start_y <= end_y) {
        float dx;
        float new_x;
        float dy;
        float new_y;
        if(env->ball_vx < 0) {
            new_x = end_x;
            dx = end_x - ball_start_x;
        } else {
            new_x = start_x - env->ball_width;
            dx = ball_start_x - new_x;
        }

        if(env->ball_vy < 0) {
            new_y = end_y;
            dy = end_y - ball_start_y;
        } else {
            new_y = start_y - env->ball_width;
            dy = ball_start_y - new_y;
        }

        if(dx < dy) {
            env->ball_vx = -env->ball_vx;
            // env->ball_y = new_y;
        } else if(dx > dy) {
            env->ball_vy = -env->ball_vy;
            // env->ball_x = new_x;
        } else {
            env->ball_vx = -env->ball_vx;
            env->ball_vy = -env->ball_vy;
            // env->ball_y = new_y;
            // env->ball_x = new_x;
        }
        return true;
    } else {
        return false;
    }

    // if((env->paddle_x_behind_offset <= env->ball_x && env->paddle_x_behind_offset >= env->ball_x + env->paddle_width) ||
    //     (env->players[player_index].behind_paddle_y <= env->ball_y && env->players[player_index].behind_paddle_y >= env->ball_y + env->paddle_height)) {
    //     return true;
    // } else {
    //     return false;
    // }
}

bool check_collision_front(SpeedHockey* env, int player_index) {
    float start_x;
    float end_x;
    float start_y = env->players[player_index].front_paddle_y;
    float end_y = env->players[player_index].front_paddle_y + env->paddle_height;
    if(player_index == 0) {
        start_x = env->paddle_x_front_offset;
        end_x = env->paddle_x_front_offset + env->paddle_width;
    } else {
        start_x = env->width - env->paddle_x_front_offset;
        end_x = env->width - env->paddle_x_front_offset + env->paddle_width;
    }
    float ball_start_x = env->ball_x;
    float ball_end_x = env->ball_x + env->ball_width;
    float ball_start_y = env->ball_y;
    float ball_end_y = env->ball_y + env->ball_height;

    if(start_x <= ball_end_x && ball_start_x <= end_x &&
        start_y <= ball_end_y && ball_start_y <= end_y) {
        float dx;
        float new_x;
        float dy;
        float new_y;
        if(env->ball_vx < 0) {
            new_x = end_x;
            dx = end_x - ball_start_x;
        } else {
            new_x = start_x - env->ball_width;
            dx = ball_start_x - new_x;
        }

        if(env->ball_vy < 0) {
            new_y = end_y;
            dy = end_y - ball_start_y;
        } else {
            new_y = start_y - env->ball_width;
            dy = ball_start_y - new_y;
        }

        if(dx < dy) {
            env->ball_vx = -env->ball_vx;
            // env->ball_y = new_y;
        } else if(dx > dy) {
            env->ball_vy = -env->ball_vy;
            // env->ball_x = new_x;
        } else {
            env->ball_vx = -env->ball_vx;
            env->ball_vy = -env->ball_vy;
            // env->ball_y = new_y;
            // env->ball_x = new_x;
        }
        return true;
    } else {
        return false;
    }
    // if((env->paddle_x_front_offset <= env->ball_x && env->paddle_x_front_offset >= env->ball_x + env->paddle_width) ||
    //     (env->players[player_index].front_paddle_y <= env->ball_y && env->players[player_index].front_paddle_y >= env->ball_y + env->paddle_height)) {
    //     return true;
    // } else {
    //     return false;
    // }
}

// Delta get_collision_d_front(SpeedHockey* env, int player_index) {
//     float dx = env->ball_x - env->paddle_x_behind_offset;
//     float dy = 
// }

void c_step(SpeedHockey* env) {
    const float MIN_Y = env->paddle_y_offset;
    const float MAX_Y = env->height - env->paddle_height - env->paddle_y_offset;
    const float BALL_MIN_Y = env->goal_offset;
    const float BALL_MAX_Y = env->height - env->ball_height - env->goal_offset;
    for (int i=0; i<PLAYER_COUNT; i++) {
        env->tick += 1;
        env->rewards[i] = 0;
        env->terminals[i] = 0;
        int action_mult = 1;
        if(i == 1){
            action_mult = -1;
        }
        if(env->continuous) {
            env->players[i].behind_paddle_dir = env->actions[i*2];
            env->players[i].front_paddle_dir = env->actions[i*2 + 1];
        } else {
            if (act_behind == 0.0) {
                env->players[i].behind_paddle_dir = 0; // still
            } else if (act_behind == 1.0) {
                env->players[i].behind_paddle_dir = 1; // up
            } else if (act_behind == 2.0) {
                env->players[i].behind_paddle_dir = -1; // down
            }

            if (act_front == 0.0) {
                env->players[i].front_paddle_dir = 0; // still
            } else if (act_front == 1.0) {
                env->players[i].front_paddle_dir = 1; // up
            } else if (act_front == 2.0) {
                env->players[i].front_paddle_dir = -1; // down
            }
        }

        env->players[i].behind_paddle_dir *= action_mult;
        env->players[i].front_paddle_dir *= action_mult;

        for (int j = 0; j < env->frameskip; j++) {
        // env->paddle_p1_y_b += env->paddle_speed * env->paddle_p1_d_b;
        // env->paddle_p1_y_f += env->paddle_speed * env->paddle_p1_d_f;
        // env->paddle_p2_y_b += env->paddle_speed * env->paddle_p2_d_b;
        // env->paddle_p2_y_f += env->paddle_speed * env->paddle_p2_d_f;
        env->players[i].behind_paddle_y += env->paddle_speed * env->players[i].behind_paddle_dir;
        env->players[i].front_paddle_y += env->paddle_speed * env->players[i].front_paddle_dir;
        
        // move opponent paddle
        // float opp_paddle_delta = env->ball_y - (env->paddle_yl + env->paddle_height / 2);
        // opp_paddle_delta = fminf(fmaxf(opp_paddle_delta, -env->paddle_speed), env->paddle_speed);
        // env->paddle_yl += opp_paddle_delta;

        // clip paddles
        env->players[i].behind_paddle_y = fminf(fmaxf(
            env->players[i].behind_paddle_y, MIN_Y), MAX_Y);
        env->players[i].front_paddle_y = fminf(fmaxf(
            env->players[i].front_paddle_y, MIN_Y), MAX_Y);
        }
        // compute_observations(env);
    }

    // move ball
    env->ball_x += env->ball_vx;
    env->ball_y += env->ball_vy;

    // handle collision with top & bottom walls
    if (env->ball_y < 0 || env->ball_y + env->ball_height > env->height) {
        env->ball_vy = -env->ball_vy;
    }

    // handle collision on left
    if (env->ball_x < 0) {
        if(env->ball_y < BALL_MIN_Y || env->ball_y > BALL_MAX_Y) {
            env->ball_vx = -env->ball_vx;
            // env->n_bounces += 1;
        // if (env->ball_y + env->ball_height > env->paddle_yl && \
        //     env->ball_y < env->paddle_yl + env->paddle_height) {
        //     // collision with paddle
        //     env->ball_vx = -env->ball_vx;
        //     env->n_bounces += 1;
        // } else {
        //     // collision with wall: WIN
        //     env->win = 1;
        //     env->score_r += 1;
        //     env->rewards[0] = 1; // agent wins
        //     if (env->score_r == env->max_score) {
        //         env->terminals[0] = 1;
        //         add_log(env);
        //         c_reset(env);
        //         return;
        //     } else {
        //         reset_round(env);
        //         return;
        //     }
        } else {
            env->score_p2 += 1;
            env->rewards[0] = -1;
            env->rewards[1] = 1;
            env->terminals[0] = 1;
            env->terminals[1] = 1;
            add_log(env);
            c_reset(env);
            // if (env->score_p2 == env->max_score) {
            //     env->terminals[0] = 1;
            //     env->terminals[1] = 1;
            //     add_log(env);
            //     c_reset(env);
            //     return;
            // } else {
            //     reset_round(env);
            //     return;
            // }
        }
    }

    if (env->ball_x + env->ball_width > env->width) {
        if(env->ball_y < BALL_MIN_Y || env->ball_y > BALL_MAX_Y) {
           env->ball_vx = -env->ball_vx;
        } else {
            env->score_p1 += 1;
            env->rewards[0] = 1;
            env->rewards[1] = -1;
            env->terminals[0] = 1;
            env->terminals[1] = 1;
            add_log(env);
            c_reset(env);
            // if (env->score_p1 == env->max_score) {
            //     env->terminals[0] = 1;
            //     env->terminals[1] = 1;
            //     add_log(env);
            //     c_reset(env);
            //     return;
            // } else {
            //     reset_round(env);
            //     return;
            // }

        }
    }

    int player_index = 0;

    if(env->ball_x > env->width / 2) {
        player_index = 1;
    }

    // Handle collisions with paddles
    if(check_collision_behind(env, player_index)) {
    //     // // float dx = env->ball_x - env->paddle_x_behind_offset;
    //     // // float dy = env->ball_y - env->players[player_index].behind_paddle_y;

    //     // if(dx < dy) {
    //     //     env->ball_vx = -env->ball_vx;
    //     // } else if(dx > dy) {
    //     //     env->ball_vy = -env->ball_vy;
    //     // } else {
    //     //     env->ball_vx = -env->ball_vx;
    //     //     env->ball_vy = -env->ball_vy;
    //     // }
    } else if(check_collision_front(env, player_index)) {
    //     // float dx = env->ball_x - env->paddle_x_front_offset;
    //     // float dy = env->ball_y - env->players[player_index].front_paddle_y;

    //     // if(dx < dy) {
    //     //     env->ball_vx = -env->ball_vx;
    //     // } else if(dx > dy) {
    //     //     env->ball_vy = -env->ball_vy;
    //     // } else {
    //     //     env->ball_vx = -env->ball_vx;
    //     //     env->ball_vy = -env->ball_vy;
    //     // }
    }

    // clip ball
    env->ball_x = fminf(fmaxf(env->ball_x, 0), env->width - env->ball_width);
    env->ball_y = fminf(fmaxf(env->ball_y, 0), env->height - env->ball_height);

    // handle collision on right (TODO duplicated code)

}

typedef struct Client Client;
struct Client {
    float width;
    float height;
    float paddle_width;
    float paddle_height;
    float ball_width;
    float ball_height;
    float x_pad;
    float goal_offset;
    float paddle_y_offset;
    float paddle_x_front_offset;
    float paddle_x_behind_offset;
    Color paddle_left_color;
    Color paddle_right_color;
    Color ball_color;
    Texture2D ball;
};

Client* make_client(SpeedHockey* env) {
    Client* client = (Client*)calloc(1, sizeof(Client));
    client->width = env->width;
    client->height = env->height;
    client->paddle_width = env->paddle_width;
    client->paddle_height = env->paddle_height;
    client->ball_width = env->ball_width;
    client->ball_height = env->ball_height;
    client->x_pad = 3*client->paddle_width;
    client->goal_offset = env->goal_offset;
    client->paddle_y_offset = env->paddle_y_offset;
    client->paddle_x_front_offset = env->paddle_x_front_offset;
    client->paddle_x_behind_offset = env->paddle_x_behind_offset;
    client->paddle_left_color = (Color){255, 0, 0, 255};
    client->paddle_right_color = (Color){0, 255, 255, 255};
    client->ball_color = (Color){255, 255, 255, 255};

    InitWindow(env->width, env->height, "PufferLib Speed Hockey");
    SetTargetFPS(60 / env->frameskip);

    client->ball = LoadTexture("resources/shared/puffers_128.png");
    return client;
}

void close_client(Client* client) {
    CloseWindow();
    free(client);
}
/*
class Rect:
    def __init__(self, x, y, width, height):
        self.x = x  # top-left x
        self.y = y  # top-left y
        self.width = width
        self.height = height

    def get_rect(self):
        return (self.x, self.y, self.width, self.height)

    def intersects(self, other):
        return not (
            self.x + self.width <= other.x or
            self.x >= other.x + other.width or
            self.y + self.height <= other.y or
            self.y >= other.y + other.height
        )

    def get_overlap(self, other):
        dx = min(self.x + self.width, other.x + other.width) - max(self.x, other.x)
        dy = min(self.y + self.height, other.y + other.height) - max(self.y, other.y)
        return dx, dy


class MovingRect(Rect):
    def __init__(self, x, y, width, height, vx, vy):
        super().__init__(x, y, width, height)
        self.vx = vx
        self.vy = vy

    def move(self):
        self.x += self.vx
        self.y += self.vy

    def check_collision_and_respond(self, other):
        if not self.intersects(other):
            return

        dx, dy = self.get_overlap(other)

        if dx > dy:  # More vertical overlap → horizontal collision
            self.vy = -self.vy
            # Clip vertically
            if self.y < other.y:
                self.y = other.y - self.height  # move up
            else:
                self.y = other.y + other.height  # move down
        elif dy > dx:  # More horizontal overlap → vertical collision
            self.vx = -self.vx
            # Clip horizontally
            if self.x < other.x:
                self.x = other.x - self.width  # move left
            else:
                self.x = other.x + other.width  # move right
        else:
            # Equal overlap – reverse both velocities
            self.vx = -self.vx
            self.vy = -self.vy
            # Clip in the direction of minimal overlap
            if dx < dy:
                # Clip horizontally
                if self.x < other.x:
                    self.x = other.x - self.width
                else:
                    self.x = other.x + other.width
            else:
                # Clip vertically
                if self.y < other.y:
                    self.y = other.y - self.height
                else:
                    self.y = other.y + other.height


# Example usage:
stationary = Rect(100, 100, 50, 50)
moving = MovingRect(120, 80, 30, 30, vx=0, vy=5)  # Moving downward

for step in range(10):
    print(f"Step {step}: MovingRect at ({moving.x}, {moving.y}), velocity=({moving.vx}, {moving.vy})")
    moving.move()
    moving.check_collision_and_respond(stationary)
*/

void c_render(SpeedHockey* env) {
    if (env->client == NULL) {
        env->client = make_client(env);
    }
    Client* client = env->client;

    if (IsKeyDown(KEY_ESCAPE)) {
        exit(0);
    }

    BeginDrawing();
    ClearBackground((Color){6, 24, 24, 255});

    // Draw left behind paddle
    DrawRectangle(
        env->players[0].behind_paddle_x,
        env->players[0].behind_paddle_y,
        client->paddle_width,
        client->paddle_height,
        client->paddle_left_color
    );

    // Draw left front paddle
    DrawRectangle(
        env->players[0].front_paddle_x,
        env->players[0].front_paddle_y,
        client->paddle_width,
        client->paddle_height,
        client->paddle_left_color
    );

    // Draw right behind paddle
    DrawRectangle(
        env->players[1].behind_paddle_x,
        env->players[1].behind_paddle_y,
        client->paddle_width,
        client->paddle_height,
        client->paddle_right_color
    );

    // Draw right front paddle
    DrawRectangle(
        env->players[1].front_paddle_x,
        env->players[1].front_paddle_y,
        client->paddle_width,
        client->paddle_height,
        client->paddle_right_color
    );

    // Draw left goal
    DrawRectangle(
        0, // x
        env->goal_offset, // y
        10, // width
        env->height - 2 * env->goal_offset, // height
        WHITE
    );

    // Draw right goal
    DrawRectangle(
        env->width - 10, // x
        env->goal_offset, // y
        10, // width
        env->height - 2 * env->goal_offset, // height
        WHITE
    );

    // Draw ball
    DrawTexturePro(
        client->ball,
        (Rectangle){
            (env->ball_vx > 0) ? 0 : 128,
            0, 128, 128,
        },
        (Rectangle){
            env->ball_x,
            env->ball_y ,
            client->ball_width,
            client->ball_height
        },
        (Vector2){0, 0},
        0,
        WHITE
    );

    //DrawFPS(10, 10);

    // Draw scores
    DrawText(
        TextFormat("%i", env->score_p1),
        client->width / 2 - 50 - MeasureText(TextFormat("%i", env->score_p1), 30) / 2,
        10, 30, (Color){0, 187, 187, 255}
    );
    DrawText(
        TextFormat("%i", env->score_p2),
        client->width / 2 + 50 - MeasureText(TextFormat("%i", env->score_p2), 30) / 2,
        10, 30, (Color){0, 187, 187, 255}
    );

    EndDrawing();
}
