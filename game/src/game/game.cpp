#include "game/game.h"
#include "game/collision.h"
#include "game/utilities.h"

#include "component/background.h"

#include <resource_manager/resource_manager.h>

using namespace std;
using namespace glm;
using namespace bko;

namespace bko
{
#define PLAYER_PADDLE_SIZE vec2{100.0, 20.0f}
#define PLAYER_PADDLE_VELOCITY vec2 {500.0f, 0.0f}
#define BALL_RADIUS 12.5f
#define BALL_VELOCITY vec2 {100.0f, -350.0f}

	// API
	Game
	game_new(GLsizei width, GLsizei height)
	{
		Game self{};

		self.state = Game::STATE_MENU;
		self.current_level = 1;
		self.lives = 3;
		self.player = Player_Paddle{};
		self.ball = Ball{};
		self.window = window_new(width, height);
		self.sprite_renderer = nullptr;

		return self;
	}

	void
	game_free(Game self)
	{
		for(auto level : self.levels)
			destruct(level);

		destruct(self.player);
		destruct(self.ball);
		destruct(self.window);
		destruct(self.sprite_renderer);
	}

	void
	game_init(Game& self)
	{
		Resource_Manager rm = IResource_Manager::get_instance();
		// load and configure shaders
		game_load_shaders();

		// configure shaders
		mat4 projection = ortho(0.0f, static_cast<GLfloat>(self.window.width), static_cast<GLfloat>(self.window.height), 0.0f, -1.0f, 1.0f);
		Program program = resource_manager_program(rm, "sprite");
		program_use(program);
		program_int_set(program, "image", 0);
		program_mat4f_set(program, "projection", projection);

		//load textures
		game_load_textures();

		//load levels
		game_load_levels(self.window, self.levels);

		// renderer initialization
		self.sprite_renderer = sprite_renderer_new(program);

		// player initialization
		self.player = player_paddle_new(resource_manager_texture(rm, "paddle"),
			vec2{ self.window.width / 2 - PLAYER_PADDLE_SIZE.x / 2, self.window.height - PLAYER_PADDLE_SIZE.y },
			PLAYER_PADDLE_SIZE,
			PLAYER_PADDLE_VELOCITY
		);

		// ball initialization
		vec2 ball_pos = self.player.sprite.position + vec2{ self.player.sprite.size.x / 2 - 12.5f, -1 * BALL_RADIUS * 2 };
		self.ball = ball_new(resource_manager_texture(rm, "ball"),
			ball_pos,
			BALL_VELOCITY,
			BALL_RADIUS,
			GL_TRUE);
	}

	inline static void
	game_process_input(Game& self, GLfloat delta_time)
	{
		if (self.state == Game::STATE_MENU)
		{
			if (glfwGetKey(self.window.handle, GLFW_KEY_E) == GLFW_PRESS)
				self.keys[GLFW_KEY_E] = GL_TRUE;

			if (glfwGetKey(self.window.handle, GLFW_KEY_E) == GLFW_RELEASE)
				self.keys[GLFW_KEY_E] = GL_FALSE;

			if (glfwGetKey(self.window.handle, GLFW_KEY_P) == GLFW_PRESS)
				self.keys[GLFW_KEY_P] = GL_TRUE;

			if (glfwGetKey(self.window.handle, GLFW_KEY_P) == GLFW_RELEASE)
				self.keys[GLFW_KEY_P] = GL_FALSE;
		}
		else if (self.state == Game::STATE_ACTIVE)
		{
			if (glfwGetKey(self.window.handle, GLFW_KEY_A) == GLFW_PRESS)
				self.keys[GLFW_KEY_A] = GL_TRUE;

			if (glfwGetKey(self.window.handle, GLFW_KEY_A) == GLFW_RELEASE)
				self.keys[GLFW_KEY_A] = GL_FALSE;

			if (glfwGetKey(self.window.handle, GLFW_KEY_D) == GLFW_PRESS)
				self.keys[GLFW_KEY_D] = GL_TRUE;

			if (glfwGetKey(self.window.handle, GLFW_KEY_D) == GLFW_RELEASE)
				self.keys[GLFW_KEY_D] = GL_FALSE;

			if (glfwGetKey(self.window.handle, GLFW_KEY_SPACE) == GLFW_PRESS)
				self.keys[GLFW_KEY_SPACE] = GL_TRUE;

			if (glfwGetKey(self.window.handle, GLFW_KEY_SPACE) == GLFW_RELEASE)
				self.keys[GLFW_KEY_SPACE] = GL_FALSE;

			GLfloat velocity = self.player.velocity.x * delta_time;
			if (self.keys[GLFW_KEY_A])
			{
				if (self.player.sprite.position.x >= 0)
				{
					self.player.sprite.position.x -= velocity;
					if (self.ball.is_stuck)
						self.ball.sprite.position.x -= velocity;
				}
			}
			if (self.keys[GLFW_KEY_D])
			{
				if (self.player.sprite.position.x <= self.window.width - self.player.sprite.size.x)
				{
					self.player.sprite.position.x += velocity;
					if (self.ball.is_stuck)
						self.ball.sprite.position.x += velocity;
				}

			}
			if (self.keys[GLFW_KEY_SPACE])
			{
				self.ball.is_stuck = GL_FALSE;
			}
		}
		else if (self.state == Game::STATE_LOSS)
		{
			if (glfwGetKey(self.window.handle, GLFW_KEY_ESCAPE) == GLFW_PRESS)
				self.keys[GLFW_KEY_ESCAPE] = GL_TRUE;

			if (glfwGetKey(self.window.handle, GLFW_KEY_ESCAPE) == GLFW_RELEASE)
				self.keys[GLFW_KEY_ESCAPE] = GL_FALSE;
		}
		else if (self.state == Game::STATE_WON)
		{
			if (glfwGetKey(self.window.handle, GLFW_KEY_E) == GLFW_PRESS)
				self.keys[GLFW_KEY_E] = GL_TRUE;

			if (glfwGetKey(self.window.handle, GLFW_KEY_E) == GLFW_RELEASE)
				self.keys[GLFW_KEY_E] = GL_FALSE;

			if (glfwGetKey(self.window.handle, GLFW_KEY_P) == GLFW_PRESS)
				self.keys[GLFW_KEY_P] = GL_TRUE;

			if (glfwGetKey(self.window.handle, GLFW_KEY_P) == GLFW_RELEASE)
				self.keys[GLFW_KEY_P] = GL_FALSE;
		}
	}

	inline static void
	game_update(Game& self, GLfloat delta_time)
	{
		if (self.state == Game::STATE_ACTIVE)
		{
			collision_detection(self, delta_time);

			if (self.ball.sprite.position.y >= self.window.height)
			{
				game_reset_level(self.levels, self.current_level);
				game_reset_player(self.player, self.window);
				game_reset_ball(self.ball, self.player);
				--self.lives;

				if (self.lives == 0)
					self.state = Game::STATE_LOSS;
			}
			else if (level_is_complete(self.levels[self.current_level - 1]))
			{
				game_reset_level(self.levels, self.current_level);
				game_reset_player(self.player, self.window);
				game_reset_ball(self.ball, self.player);
				++self.current_level;

				if (self.current_level == 5)
					self.state = Game::STATE_WON;
			}
		}
	}

	inline static void
	game_render(Game& self)
	{
		if (self.state == Game::STATE_MENU)
		{
			// render start menu
			Texture menu = resource_manager_texture(IResource_Manager::get_instance(), "menu");
			Background background = background_new(menu, vec2{ 0.0f, 0.0f }, vec2{ self.window.width, self.window.height });
			sprite_renderer_render(self.sprite_renderer, background.sprite);

			if(self.keys[GLFW_KEY_E])
				glfwSetWindowShouldClose(self.window.handle, true);

			if (self.keys[GLFW_KEY_P])
				self.state = Game::STATE_ACTIVE;
		}
		else if (self.state == Game::STATE_ACTIVE)
		{
			// render background
			Texture bkg = resource_manager_texture(IResource_Manager::get_instance(), "background");
			Background background = background_new(bkg, vec2{ 0.0f, 0.0f }, vec2{ self.window.width, self.window.height });
			sprite_renderer_render(self.sprite_renderer, background.sprite);

			// render level
			for (auto brick : self.levels[self.current_level - 1].bricks)
				if (!brick.is_destroyed)
					sprite_renderer_render(self.sprite_renderer, brick.sprite);

			//render player paddle
			sprite_renderer_render(self.sprite_renderer, self.player.sprite);

			//render ball
			sprite_renderer_render(self.sprite_renderer, self.ball.sprite);
		}
		else if (self.state == Game::STATE_LOSS)
		{
			// render Loss texture
			Texture loss = resource_manager_texture(IResource_Manager::get_instance(), "loss");
			Background background = background_new(loss, vec2{ 0.0f, 0.0f }, vec2{ self.window.width, self.window.height });
			sprite_renderer_render(self.sprite_renderer, background.sprite);

			if (self.keys[GLFW_KEY_ESCAPE])
				glfwSetWindowShouldClose(self.window.handle, true);

		}
		else if (self.state == Game::STATE_WON)
		{
			// render Loss texture
			Texture won = resource_manager_texture(IResource_Manager::get_instance(), "won");
			Background background = background_new(won, vec2{ 0.0f, 0.0f }, vec2{ self.window.width, self.window.height });
			sprite_renderer_render(self.sprite_renderer, background.sprite);

			if (self.keys[GLFW_KEY_E])
				glfwSetWindowShouldClose(self.window.handle, true);

			if (self.keys[GLFW_KEY_P])
				self.state = Game::STATE_ACTIVE;
		}
	}

	void
	game_run(Game self)
	{
		float current_frame;
		float delta_time = 0.0f;
		float last_frame = 0.0f;
		// render loop
		while (!glfwWindowShouldClose(self.window.handle))
		{
			current_frame = glfwGetTime();
			delta_time = current_frame - last_frame;
			last_frame = current_frame;
			// input
			game_process_input(self, delta_time);

			//update
			game_update(self, delta_time);

			// render stuff
			game_render(self);

			// swap buffers
			glfwSwapBuffers(self.window.handle);

			// poll events
			glfwPollEvents();
		}
	}

}