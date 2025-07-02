'''High-perf Speedhockey

Inspired from  
& https://jair.org/index.php/jair/article/view/10819/25823
& https://www.youtube.com/watch?v=PSQt5KGv7Vk
'''

import numpy as np
import gymnasium

import pufferlib
from pufferlib.ocean.speed_hockey import binding
#import binding

class Speedhockey(pufferlib.PufferEnv):
    def __init__(self, num_envs=1, num_agents=2, render_mode=None,
            width=700, height=350, paddle_width=20, paddle_height=70,
            ball_width=32, ball_height=32, paddle_speed=8,
            ball_initial_speed_x=10, ball_initial_speed_y=1,
            ball_speed_y_increment=3, ball_max_speed_y=13,
            max_score=21, goal_offset=30, paddle_y_offset=20,
            paddle_x_behind_offset=30, paddle_x_front_offset=100,
            frameskip=1, continuous=False, log_interval=128, buf=None, seed=0):
        self.single_observation_space = gymnasium.spaces.Box(
            low=0, high=1, shape=(10,), dtype=np.float32,
        )
        # if continuous:
        #     self.single_action_space = gymnasium.spaces.Box(
        #         low=-1, high=1,  dtype=np.float32,
        #     )
        # else:
        self.single_action_space = gymnasium.spaces.Discrete(7)
        num_agents = 2
        
        self.render_mode = render_mode
        self.num_agents = num_envs*num_agents
        self.continuous = continuous
        self.log_interval = log_interval
        self.tick = 0

        super().__init__(buf)
        # if continuous:
        #     self.actions = self.actions.flatten()
        # else:
        self.actions = self.actions.astype(np.float32)

        c_envs = []
        for i in range(num_envs):
            c_env = binding.env_init(
                self.observations[i*num_agents:(i+1)*num_agents],
                self.actions[i*num_agents:(i+1)*num_agents],
                self.rewards[i*num_agents:(i+1)*num_agents],
                self.terminals[i*num_agents:(i+1)*num_agents],
                self.truncations[i*num_agents:(i+1)*num_agents],
                seed,
                width=width, height=height, paddle_width=paddle_width, paddle_height=paddle_height,
                ball_width=ball_width, ball_height=ball_height, paddle_speed=paddle_speed,
                ball_initial_speed_x=ball_initial_speed_x, ball_initial_speed_y=ball_initial_speed_y,
                ball_speed_y_increment=ball_speed_y_increment, ball_max_speed_y=ball_max_speed_y,
                max_score=max_score, goal_offset=goal_offset, paddle_y_offset=paddle_y_offset,
                paddle_x_behind_offset=paddle_x_behind_offset, paddle_x_front_offset=paddle_x_front_offset,
                frameskip=frameskip, continuous=continuous)
            c_envs.append(c_env)

        self.c_envs = binding.vectorize(*c_envs)

    def reset(self, seed=0):
        binding.vec_reset(self.c_envs, seed)
        self.tick = 0
        return self.observations, []

    def step(self, actions):
        if  self.continuous:
            self.actions[:] = np.clip(actions.flatten(), -1.0, 1.0)
        else: 
            self.actions[:] = actions
 
        self.tick += 1
        binding.vec_step(self.c_envs)

        info = []
        if self.tick % self.log_interval == 0:
            info.append(binding.vec_log(self.c_envs))

        return (self.observations, self.rewards,
            self.terminals, self.truncations, info)

    def render(self):
        binding.vec_render(self.c_envs, 0)

    def close(self):
        binding.vec_close(self.c_envs)

if __name__ == "__main__":
    env = Speedhockey()
    env.reset()
    tick = 0
    timeout=30

    tot_agents = env.num_agents
    actions = np.random.randint(0,5,(1024,tot_agents))

    import time 
    start = time.time()
    # while time.time() - start < timeout:
    while tick < 500:
        atns = actions[tick % 1024]
        env.step(atns)
        if -1 in env.rewards:
            breakpoint()
        # env.render()
        tick += 1

    print(f'SPS: {int(tot_agents * tick / (time.time() - start)):_}')

    env.close()

#from cy_pong import CyPong
# class CythonSpeedhockey(pufferlib.PufferEnv):
#     def __init__(self, num_envs=1, render_mode=None,
#             width=500, height=640, paddle_width=20, paddle_height=70,
#             ball_width=32, ball_height=32, paddle_speed=8,
#             ball_initial_speed_x=10, ball_initial_speed_y=1,
#             ball_speed_y_increment=3, ball_max_speed_y=13,
#             max_score=21, frameskip=1, continuous=False, report_interval=128, buf=None):
#         self.single_observation_space = gymnasium.spaces.Box(
#             low=0, high=1, shape=(8,), dtype=np.float32,
#         )
#         if continuous:
#             self.single_action_space = gymnasium.spaces.Box(
#                 low=-1, high=1,  dtype=np.float32,
#             )
#         else:
#             self.single_action_space = gymnasium.spaces.Discrete(3)
        
#         self.render_mode = render_mode
#         self.num_agents = num_envs
#         self.continuous = continuous
#         self.report_interval = report_interval
#         self.human_action = None
#         self.tick = 0

#         super().__init__(buf)
#         if continuous:
#             self.actions = self.actions.flatten()
#         else:
#             self.actions = self.actions.astype(np.float32)
#         self.c_envs = CyPong(self.observations, self.actions, self.rewards,
#             self.terminals, num_envs, width, height,
#             paddle_width, paddle_height, ball_width, ball_height,
#             paddle_speed, ball_initial_speed_x, ball_initial_speed_y,
#             ball_max_speed_y, ball_speed_y_increment, max_score, frameskip, continuous)
 
#     def reset(self, seed=None):
#         self.tick = 0
#         self.c_envs.reset()
#         return self.observations, []

#     def step(self, actions):
#         if  self.continuous:
#             self.actions[:] = np.clip(actions.flatten(), -1.0, 1.0)
#         else: 
#             self.actions[:] = actions
        
#         self.c_envs.step()
#         info = []
#         self.tick += 1
#         return (self.observations, self.rewards,
#             self.terminals, self.truncations, info)

#     def render(self):
#         self.c_envs.render()

#     def close(self):
#         self.c_envs.close()

# def test_performance(cls, timeout=10, atn_cache=1024):
#     env = cls(num_envs=1000)
#     env.reset()
#     tick = 0

#     actions = np.random.randint(0, 2, (atn_cache, env.num_agents))

#     import time
#     start = time.time()
#     while time.time() - start < timeout:
#         atn = actions[tick % atn_cache]
#         env.step(atn)
#         tick += 1

#     print(f'{env.__class__.__name__}: SPS: {env.num_agents * tick / (time.time() - start)}')
