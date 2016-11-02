////////////////////////////////////////////////////////////////////////////////
//
// (C) Andy Thomason 2012-2014
//
// Modular Framework for OpenGLES2 rendering on multiple platforms.
//
// invaderer example: simple game with sprites and sounds
//
// Level: 1
//
// Demonstrates:
//   Basic framework app
//   Shaders
//   Basic Matrices
//   Simple game mechanics
//   Texture loaded from GIF file
//   Audio
//

#include <ctime>
#include <iostream>

using namespace std;


namespace octet {
	class sprite {
		// where is our sprite (overkill for a 2D game!)
		mat4t modelToWorld;

		// half the width of the sprite
		float HalfWidth;

		// half the height of the sprite
		float HalfHeight;

		// what texture is on our sprite
		int texture;

		// true if this sprite is enabled.
		bool enabled;

	public:
		float x;
		float y;

		sprite() {
			texture = 0;
			enabled = true;
		}

		void init(int _texture, float X, float Y, float w, float h) {
			x = X; y = Y;
			modelToWorld.loadIdentity();
			modelToWorld.translate(X, Y, 0);
			HalfWidth = w * 0.5f;
			HalfHeight = h * 0.5f;
			texture = _texture;
			enabled = true;
		}

		void render(texture_shader &shader, mat4t &cameraToWorld) {
			// invisible sprite... used for gameplay.
			if (!texture) return;

			// build a projection matrix: model -> world -> camera -> projection
			// the projection space is the cube -1 <= x/w, y/w, z/w <= 1
			mat4t modelToProjection = mat4t::build_projection_matrix(modelToWorld, cameraToWorld);

			// set up opengl to draw textured triangles using sampler 0 (GL_TEXTURE0)
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, texture);

			// use "old skool" rendering
			//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
			//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			shader.render(modelToProjection, 0);

			// this is an array of the positions of the corners of the sprite in 3D
			// a straight "float" here means this array is being generated here at runtime.
			float vertices[] = {
				-HalfWidth, -HalfHeight, 0,
				HalfWidth, -HalfHeight, 0,
				HalfWidth,  HalfHeight, 0,
				-HalfWidth, HalfHeight, 0,
			};

			// attribute_pos (=0) is position of each corner
			// each corner has 3 floats (x, y, z)
			// there is no gap between the 3 floats and hence the stride is 3*sizeof(float)
			glVertexAttribPointer(attribute_pos, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)vertices);
			glEnableVertexAttribArray(attribute_pos);

			// this is an array of the positions of the corners of the texture in 2D
			static const float uvs[] = {
				0,  0,
				1,  0,
				1,  1,
				0,  1,
			};

			// attribute_uv is position in the texture of each corner
			// each corner (vertex) has 2 floats (x, y)
			// there is no gap between the 2 floats and hence the stride is 2*sizeof(float)
			glVertexAttribPointer(attribute_uv, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)uvs);
			glEnableVertexAttribArray(attribute_uv);

			// finally, draw the sprite (4 vertices)
			glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		}

		// move the object
		void translate(float X, float Y) {
			x += X;
			y += Y;
			modelToWorld.translate(X, Y, 0);
		}

		// move the object
		void rotate(float angle) {
			modelToWorld.rotate(angle, 0, 0, 1);
		}

		// move the object
		void change_sprite(int temp_sprite) {
			texture = temp_sprite;
		}

		bool check_collision(sprite &another_sprite) {
			if (abs(another_sprite.x - x) <= 2*HalfWidth && abs(another_sprite.y - y) <= 2*HalfHeight)
			  {
				  return true;
			  }

			return false;
		}
		
		// return true if this sprite collides with another.
		// note the "const"s which say we do not modify either sprite
		bool collides_with(const sprite &rhs) const {
			float dx = rhs.modelToWorld[3][0] - modelToWorld[3][0];
			float dy = rhs.modelToWorld[3][1] - modelToWorld[3][1];

			// both distances have to be under the sum of the halfwidths
			// for a collision
			return
				(fabsf(dx) < HalfWidth + rhs.HalfWidth) &&
				(fabsf(dy) < HalfHeight + rhs.HalfHeight)
				;
		}

	};

	class invaderers_app : public octet::app {
#pragma region VARIABLES

		// Matrix to transform points in our camera space to the world.
		// This lets us move our camera
		mat4t cameraToWorld;

		// shader to draw a textured triangle
		texture_shader texture_shader_;

		// game state
		bool game_over;
		bool game_fail;
		bool game_start;
		bool start_the_game;
		bool start_chasing = false;
		bool speed_up_flag = false;
		bool start_moving = false;

		// big array of sprites
		sprite sprites[500];

		// number of guards and traps (blue diamonds)
		enum {
			num_sound_sources = 8,
			easy = 30,
			medium = 50,
			hard = 10,
			num_guards = 4,
			num_trap = medium,
			num_diamond = 1,
			num_borders = 4,
		};

		// sounds
		ALuint door;
		ALuint bang;
		unsigned cur_source;
		ALuint sources[num_sound_sources];

		// speed of enemy
		float guard_velocity;

		ALuint get_sound_source() { return sources[cur_source++ % num_sound_sources]; }
		
		
		int num_sprites;
		int thief_sprite_index;
		int first_guard_sprite_index;
		int first_trap_sprite_index;
		int first_diamond_sprite_index;
		int win_sprite_index;
		int lose_sprite_index;
		int first_border_index;
		int start_button_index;


		GLuint empty_trap;
		GLuint trap;
		int frames = 0;
		
		// random number generator
		class random randomizer;

		// a texture for our text
		GLuint font_texture;

		// information for our text
		bitmap_font font;

		float set_speed_guard = 0.02f;

		//ALuint get_sound_source() { return sources[cur_source++ % num_sound_sources]; }

#pragma endregion
#pragma region FUNCTIONS

		// use the keyboard to move the thief
		void move_thief() {
			const float ship_speed = 0.3f;

			// left and right arrows
			if (is_key_down(key_left)) {
				sprites[thief_sprite_index].translate(-ship_speed, 0);	
				if (sprites[thief_sprite_index].collides_with(sprites[first_border_index + 2])) {
					sprites[thief_sprite_index].translate(+ship_speed, 0);
				}
			}
			else if (is_key_down(key_right)) {
				sprites[thief_sprite_index].translate(+ship_speed, 0);
				if (sprites[thief_sprite_index].collides_with(sprites[first_border_index + 3])) {
					sprites[thief_sprite_index].translate(-ship_speed, 0);
				}
			}
			else if (is_key_down(key_up)) {
				sprites[thief_sprite_index].translate(0, +ship_speed);
				if (sprites[thief_sprite_index].collides_with(sprites[first_border_index + 1])) {
					sprites[thief_sprite_index].translate(0,-ship_speed);
				}
			}
			else if (is_key_down(key_down)) {
				sprites[thief_sprite_index].translate(0, -ship_speed);
				if (sprites[thief_sprite_index].collides_with(sprites[first_border_index])) {
					sprites[thief_sprite_index].translate(0,+ship_speed);
				}
			}
			
		}

		void critical_interactions () {

			// for start the game
	
					game_start = sprites[thief_sprite_index].check_collision(sprites[start_button_index]);
					if (game_start)
					{
						printf("you win!");
						sprites[win_sprite_index].translate(-20, 0);
						return;
					}
	

			// if our thief collides with the true diamond! 
			for (int j = 0; j < num_diamond; j++) {
				game_over= sprites[thief_sprite_index].check_collision(sprites[first_diamond_sprite_index + j]);
				if (game_over)
				{
					printf("you win!");
					sprites[win_sprite_index].translate(-20, 0);
					return;
				}
				
			}


			// if our thief collides with a false diamond, a trap!! 
			if (!start_chasing) {
				for (int j = 0; j < num_trap; j++)
				{
					start_chasing = sprites[thief_sprite_index].check_collision(sprites[first_trap_sprite_index + j]);
					if (start_chasing)
						break;
				}
			}

			//speed up and sound for each trap
			for (int j = 0; j < num_trap; j++)
			{
				speed_up_flag = sprites[thief_sprite_index].check_collision(sprites[first_trap_sprite_index + j]);
				if (speed_up_flag)
				{
					ALuint source = get_sound_source();
					alSourcei(source, AL_BUFFER, bang);
					alSourcePlay(source);
					set_speed_guard += 0.005;
				}
			}

			// if our thief collides with a guard 
			for (int j = 0; j < num_guards; j++) {
				game_fail = sprites[thief_sprite_index].check_collision(sprites[first_guard_sprite_index + j]);
				if (game_fail)
				{
					printf("you lose");
					sprites[lose_sprite_index].translate(-20, 0);
					return;
				}
			
			}

		}

		void move_guard(float dx, float dy) {
			for (int j = 0; j != num_guards; ++j) {
				sprite &guard = sprites[first_guard_sprite_index+j];
				guard.translate(dx, dy);

			}
		}

		bool guard_collide(sprite &border) {
			for (int j = 0; j != num_guards; ++j) {
				sprite &guard = sprites[first_guard_sprite_index + j];
				if (guard.collides_with(border)) {
					return true;
				}
			}
			return false;
		}


		// how guards move in a chasing
		void chasing() {
			for (int j = 0; j < num_guards; j++) {
				float movement_x = set_speed_guard;
				float movement_y = set_speed_guard;
				if (sprites[thief_sprite_index].x < sprites[first_guard_sprite_index+j].x)
					movement_x = movement_x * -1;
				if (sprites[thief_sprite_index].y < sprites[first_guard_sprite_index+j].y)
					movement_y = movement_y * -1;
				sprites[first_guard_sprite_index+j].translate(movement_x, movement_y);
			}
		}

		void draw_text(texture_shader &shader, float x, float y, float scale, const char *text) {
			mat4t modelToWorld;
			modelToWorld.loadIdentity();
			modelToWorld.translate(x, y, 0);
			modelToWorld.scale(scale, scale, 1);
			mat4t modelToProjection = mat4t::build_projection_matrix(modelToWorld, cameraToWorld);

			/*mat4t tmp;
			glLoadIdentity();
			glTranslatef(x, y, 0);
			glGetFloatv(GL_MODELVIEW_MATRIX, (float*)&tmp);
			glScalef(scale, scale, 1);
			glGetFloatv(GL_MODELVIEW_MATRIX, (float*)&tmp);*/

			enum { max_quads = 32 };
			bitmap_font::vertex vertices[max_quads * 4];
			uint32_t indices[max_quads * 6];
			aabb bb(vec3(0, 0, 0), vec3(256, 256, 0));

			unsigned num_quads = font.build_mesh(bb, vertices, indices, max_quads, text, 0);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, font_texture);

			shader.render(modelToProjection, 0);

			glVertexAttribPointer(attribute_pos, 3, GL_FLOAT, GL_FALSE, sizeof(bitmap_font::vertex), (void*)&vertices[0].x);
			glEnableVertexAttribArray(attribute_pos);
			glVertexAttribPointer(attribute_uv, 3, GL_FLOAT, GL_FALSE, sizeof(bitmap_font::vertex), (void*)&vertices[0].u);
			glEnableVertexAttribArray(attribute_uv);

			glDrawElements(GL_TRIANGLES, num_quads * 6, GL_UNSIGNED_INT, indices);
		}

		void turn_sprites_off() {
			for (int j = 0; j < num_trap; j++) {
				sprites[first_trap_sprite_index + j].change_sprite(empty_trap);
			}


			GLuint background = resource_dict::get_texture_handle(GL_RGBA, "assets/diamonds/background_dark.gif");
			sprites[0].init(background, 0, 0, 20, 20);

			ALuint source = get_sound_source();
			alSourcei(source, AL_BUFFER, door);
			alSourcePlay(source);
		}

		void start_the_game() {
			
			int current_sprite = 1;
			srand(time(NULL));

			GLuint background = resource_dict::get_texture_handle(GL_RGBA, "assets/diamonds/background.gif");
			sprites[0].init(background, 0, 0, 20, 20);

			empty_trap = resource_dict::get_texture_handle(GL_RGBA, "assets/diamonds/empty.gif");
			trap = resource_dict::get_texture_handle(GL_RGBA, "assets/diamonds/trap.gif");
			first_trap_sprite_index = current_sprite;
			for (int j = 0; j<num_trap; j++) {
				int r1 = rand() % 19 - 9,
					r2 = rand() % 16 - 8;
				sprites[current_sprite++].init(trap, r1, r2, 1.5, 1.5);
			}

			GLuint guard = resource_dict::get_texture_handle(GL_RGBA, "assets/diamonds/guard.gif");
			first_guard_sprite_index = current_sprite;
			int guard_position[9] = {
				0,4,
				4,7,
				-2,-4,
				-4,-8
			};
			int index = 0;
			for (int j = 0; j<num_guards; j++) {
				sprites[current_sprite++].init(guard, guard_position[index], guard_position[index + 1], 1, 1);
				index += 2;
			}

			GLuint diamond = resource_dict::get_texture_handle(GL_RGBA, "assets/diamonds/diamond.gif");
			first_diamond_sprite_index = current_sprite;
			for (int j = 0; j < num_diamond; j++) {
				int r1 = rand() % 19 - 9,
					r2 = rand() % -9;
				sprites[current_sprite++].init(diamond, r1, r2, 1, 1);
			}

			GLuint GameOver = resource_dict::get_texture_handle(GL_RGBA, "assets/diamonds/win.gif");
			win_sprite_index = current_sprite;
			sprites[current_sprite++].init(GameOver, 20, 0, 3, 1);

			GLuint GameFail = resource_dict::get_texture_handle(GL_RGBA, "assets/diamonds/lose.gif");
			lose_sprite_index = current_sprite;
			sprites[current_sprite++].init(GameFail, 20, 0, 3, 1);

			// set the border to white for clarity
			GLuint white = resource_dict::get_texture_handle(GL_RGB, "assets/diamonds/brick2.gif");
			first_border_index = current_sprite;
			sprites[current_sprite++].init(white, 0, -10, 20, 1);
			sprites[current_sprite++].init(white, 0, 10, 20, 1);
			white = resource_dict::get_texture_handle(GL_RGB, "assets/diamonds/brick.gif");
			sprites[current_sprite++].init(white, -10, 0, 1, 20);
			sprites[current_sprite++].init(white, 10, 0, 1, 20);

			// sounds
			door = resource_dict::get_sound_handle(AL_FORMAT_MONO16, "assets/diamonds/door.wav");
			bang = resource_dict::get_sound_handle(AL_FORMAT_MONO16, "assets/invaderers/bang.wav");

			cur_source = 0;
			alGenSources(num_sound_sources, sources);

			num_sprites = current_sprite;

			guard_velocity = 0.05f;

			}


#pragma endregion
#pragma region MAIN GAME
	public:

		// this is called when we construct the class
		invaderers_app(int argc, char **argv) : app(argc, argv), font(512, 256, "assets/big.fnt") {
		}

		// this is called once OpenGL is initialized
		void app_init() {
			// set up the shader
			texture_shader_.init();

			// set up the matrices with a camera 5 units from the origin
			cameraToWorld.loadIdentity();
			cameraToWorld.translate(0, 0, 10);

			font_texture = resource_dict::get_texture_handle(GL_RGBA, "assets/big_0.gif");
			//load all ship textures
			
			//file_reader::set_up_totals(num_guards,num_trap,num_diamond);

			int current_sprite = 1;

			GLuint start_button = resource_dict::get_texture_handle(GL_RGBA, "assets/diamonds/start.gif");
			start_button_index = current_sprite;
			sprites[current_sprite++].init(start_button, -6,8, 1, 1);
			
			GLuint thief = resource_dict::get_texture_handle(GL_RGBA, "assets/diamonds/thief.gif");
			thief_sprite_index = current_sprite;
				int r1 = rand() % 18 - 9,
					r2 = 9;
				sprites[current_sprite++].init(thief, r1, r2, 1, 1);

			game_start = false;
			game_over = false;
			game_fail = false;
		}

		// called every frame to move things
		void simulate() {
			
			if (game_start) {
				start_the_game();
			}

			if (game_over) {
				return;
			}

			 if (game_fail) {
				 return;
			 }
			 if (start_moving) {
				 move_thief();
			}
			
			critical_interactions();

			if (start_chasing) {
				chasing();
			}
			else
			{
				move_guard(guard_velocity, 0);
			}

			sprite &border = sprites[first_border_index + (guard_velocity < 0 ? 2 : 3)];
			if (guard_collide(border)) {
				guard_velocity = -guard_velocity;
			}

			if (frames == 60)
			{
				turn_sprites_off();
				start_moving = true;
			}

			frames++;


		}

#pragma endregion

		// this is called to draw the world
		void draw_world(int x, int y, int w, int h) {
			simulate();

			// set a viewport - includes whole window area
			glViewport(x, y, w, h);

			// clear the background to black
			glClearColor(0, 0, 0, 1);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			// don't allow Z buffer depth testing (closer objects are always drawn in front of far ones)
			glDisable(GL_DEPTH_TEST);

			// allow alpha blend (transparency when alpha channel is 0)
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			// draw all the sprites
			for (int i = 0; i < num_sprites; ++i) {
				sprites[i].render(texture_shader_, cameraToWorld);
			}

			//char score_text[32];
			//sprintf(score_text, "score: %d   lives: %d\n", score, num_lives);
			//draw_text(texture_shader_, -1.75f, 2, 1.0f / 256, score_text);

			// move the listener with the camera
			vec4 &cpos = cameraToWorld.w();
			alListener3f(AL_POSITION, cpos.x(), cpos.y(), cpos.z());
		}
	};
}
