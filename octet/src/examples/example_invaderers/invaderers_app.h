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
		bool start_chasing = false;
		bool speed_up_flag = false;
		bool start_moving = false;
		bool show_about_text = false;
		bool chasing_music = true;
			
		// big array of sprites
		sprite sprites[500];

		// number of guards and traps (blue diamonds)
		enum {
			num_sound_sources=20,
			easy = 30,
			medium = 50,
			hard = 80,
			num_guards = 8,
			num_trap = hard,
			num_diamond = 1,
			num_borders = 4,
		};

		// sounds
		ALuint door;
		ALuint bang;
		ALuint menu_music;
		ALuint chase_music;
		ALuint chase_music_file;
		unsigned cur_source;
		ALuint audio_inicio;
		ALuint win_sound_file;
		ALuint win_sound;
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
		int r1, r2;
		int current_sprite=1;


// GLunits 

		GLuint empty_trap;
		GLuint trap;
		GLuint background;
		GLuint thief;
		GLuint guard;
		GLuint diamond;
		GLuint GameOver;
		GLuint GameFail;
		GLuint borde_hor;
		GLuint borde_vert;
		GLuint start_button_monito_lindo;
		GLuint logo_glunit;
		GLuint about_glunit;
		GLuint about_button_glunit;

		//sprite
		sprite start_button;
		sprite thief_sprite;
		sprite logo;
		sprite about;
		sprite about_button;

		int frames = 0;
		float speed[8]{0.06f,-0.06f,0.06f,0.06f,0.06f,-0.06f,0.06f,0.06f };
		
		// random number generator
		class random randomizer;

		// a texture for our text
		GLuint font_texture;

		// information for our text
		bitmap_font font;

		float set_speed_guard = 0.06f;

		//ALuint get_sound_source() { return sources[cur_source++ % num_sound_sources]; }

#pragma endregion
#pragma region FUNCTIONS

		// use the keyboard to move the thief
		void move_thief() {
			const float ship_speed = 0.3f;

			// left and right arrows
			if (is_key_down(key_left)) {
				thief_sprite.translate(-ship_speed, 0);
				thief_sprite.change_sprite(resource_dict::get_texture_handle(GL_RGBA, "assets/diamonds/thief_i.gif"));
				if (thief_sprite.collides_with(sprites[first_border_index + 2])) {
					thief_sprite.translate(+ship_speed, 0);
				}
			}
			else if (is_key_down(key_right)) {
				thief_sprite.translate(+ship_speed, 0);
				thief_sprite.change_sprite(resource_dict::get_texture_handle(GL_RGBA, "assets/diamonds/thief_d.gif"));
				if (thief_sprite.collides_with(sprites[first_border_index + 3])) {
					thief_sprite.translate(-ship_speed, 0);
				}
			}
			else if (is_key_down(key_up)) {
				thief_sprite.translate(0, +ship_speed);
				thief_sprite.change_sprite(resource_dict::get_texture_handle(GL_RGBA, "assets/diamonds/thief_arr.gif"));
				if (thief_sprite.collides_with(sprites[first_border_index + 1])) {
					thief_sprite.translate(0,-ship_speed);
				}
			}
			else if (is_key_down(key_down)) {
				thief_sprite.translate(0, -ship_speed);
				thief_sprite.change_sprite(resource_dict::get_texture_handle(GL_RGBA, "assets/diamonds/thief_ab.gif"));
				if (thief_sprite.collides_with(sprites[first_border_index])) {
					thief_sprite.translate(0,+ship_speed);
				}
			}
			
		}

		// use the keyboard to move the thief
		void move_thief_intro() {
			const float ship_speed = 0.3f;

			// left and right arrows
			if (is_key_down(key_left)) {
				thief_sprite.translate(-ship_speed, 0);
				thief_sprite.change_sprite(resource_dict::get_texture_handle(GL_RGBA, "assets/diamonds/thief_i.gif"));
			}
			else if (is_key_down(key_right)) {
				thief_sprite.translate(+ship_speed, 0);
				thief_sprite.change_sprite(resource_dict::get_texture_handle(GL_RGBA, "assets/diamonds/thief_d.gif"));
			}
			else if (is_key_down(key_up)) {
				thief_sprite.translate(0, +ship_speed);
				thief_sprite.change_sprite(resource_dict::get_texture_handle(GL_RGBA, "assets/diamonds/thief_arr.gif"));
			}
			else if (is_key_down(key_down)) {
				thief_sprite.translate(0, -ship_speed);
				thief_sprite.change_sprite(resource_dict::get_texture_handle(GL_RGBA, "assets/diamonds/thief_ab.gif"));
			}

		}

		void go() {
			if (thief_sprite.collides_with(start_button)) {
				thief_sprite.translate(r1 = rand() % 18 - 9, r2 = 9);
				thief_sprite.init(thief, r1, r2, 1, 1);
				start_game();
				game_start = true;
			}
		}
		
		void start_game() {
			
			current_sprite = 1;
	
			sprites[0].init(background, 0, 0, 20, 20);
			
			first_trap_sprite_index = current_sprite;
			for (int j = 0; j<num_trap; j++) {
				int r1 = rand() % 19 - 9,
					r2 = rand() % 16 - 8;
				
				sprites[current_sprite++].init(trap, r1, r2, 1.0, 1.0);
			}

			first_guard_sprite_index = current_sprite;
			int guard_position[16] = {
				0,4,
				4,7,
				-2,-4,
				-4,-8,
				0,2,
				6,3,
				-3,-5,
				-5,-9
			};
			int index = 0;
			for (int j = 0; j<num_guards; j++) {
				sprites[current_sprite++].init(guard, guard_position[index], guard_position[index + 1], 1, 1);
				index += 2;
			}

			first_diamond_sprite_index = current_sprite;
			r1 = rand() % 19 - 9;
			r2 = -9;
			sprites[current_sprite++].init(diamond, r1, r2, 1, 1);


			win_sprite_index = current_sprite;
			sprites[current_sprite++].init(GameOver, 20, 0, 6, 2);

			lose_sprite_index = current_sprite;
			sprites[current_sprite++].init(GameFail, 20, 0, 6, 2);

			first_border_index = current_sprite;
			sprites[current_sprite++].init(borde_hor, 0, -10, 20, 1);
			sprites[current_sprite++].init(borde_hor, 0, 10, 20, 1);
			sprites[current_sprite++].init(borde_vert, -10, 0, 1, 20);
			sprites[current_sprite++].init(borde_vert, 10, 0, 1, 20);

			num_sprites = current_sprite;

			logo.translate(-100, 0);
			about.translate(-100, 0);
			start_button.translate(-100, 0);
			about_button.translate(-100, 0);
		}

		void critical_interactions () {

			// if our thief collides with the true diamond! 

				game_over= thief_sprite.check_collision(sprites[first_diamond_sprite_index]);
				if (game_over)
				{
					printf("you win!");
					sprites[win_sprite_index].translate(-20, 0);

					alSourcePause(chase_music);
					alSourcePause(audio_inicio);

					win_sound = get_sound_source();
					alSourcei(win_sound, AL_BUFFER, win_sound_file);
					alSourcePlay(win_sound);

					return;
				}
				
	

			// if our thief collides with a false diamond, a trap!! 
			if (!start_chasing) {
				for (int j = 0; j < num_trap; j++)
				{
					start_chasing = thief_sprite.check_collision(sprites[first_trap_sprite_index + j]);
					if (start_chasing)
						break;
				}

			}

			//speed up for each trap
			for (int j = 0; j < num_trap; j++)
			{
				speed_up_flag = thief_sprite.check_collision(sprites[first_trap_sprite_index + j]);
				if (speed_up_flag)
				{
					set_speed_guard += 0.001;
				}
			}

			// if our thief collides with a guard 
			for (int j = 0; j < num_guards; j++) {
				game_fail = thief_sprite.check_collision(sprites[first_guard_sprite_index + j]);
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
				guard.translate(speed[j], dy);

			}
		}

		void guard_collide(sprite &border_i, sprite &border_d) {
			for (int j = 0; j != num_guards; ++j) {
				sprite &guard = sprites[first_guard_sprite_index + j];
				if (guard.collides_with(border_i) || guard.collides_with(border_d)) {
					speed[j] = speed[j] * -1;
				}
			}
		}

		// how guards move in a chasing
		void chasing() {
			for (int j = 0; j < num_guards; j++) {
				float movement_x = set_speed_guard;
				float movement_y = set_speed_guard;
				if (thief_sprite.x < sprites[first_guard_sprite_index+j].x)
					movement_x = movement_x * -1;
				if (thief_sprite.y < sprites[first_guard_sprite_index+j].y)
					movement_y = movement_y * -1;
				sprites[first_guard_sprite_index+j].translate(movement_x, movement_y);
				sprites[first_guard_sprite_index + j].change_sprite(resource_dict::get_texture_handle(GL_RGBA, "assets/diamonds/guard2.gif"));
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

		void switch_sounds() {
			if (chasing_music) {

				alSourcePause(audio_inicio);

				chase_music = get_sound_source();
				alSourcei(chase_music, AL_BUFFER, chase_music_file);
				alSourcePlay(chase_music);
				chasing_music = false;
			}
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

			srand(time(NULL));

			// set up the matrices with a camera 5 units from the origin
			cameraToWorld.loadIdentity();
			cameraToWorld.translate(0, 0, 10);

			//generate the menu
			start_button_monito_lindo = resource_dict::get_texture_handle(GL_RGBA, "assets/diamonds/start.gif");
			start_button.init(start_button_monito_lindo, 7, -8, 4, 2);

			about_button_glunit=resource_dict::get_texture_handle(GL_RGBA, "assets/diamonds/about.gif");
			about_button.init(about_button_glunit, -7, -8, 4, 2);

			about_glunit = resource_dict::get_texture_handle(GL_RGBA, "assets/diamonds/text.gif");
			about.init(about_glunit, 50, 0, 20, 20);

			logo_glunit = resource_dict::get_texture_handle(GL_RGBA, "assets/diamonds/logo.gif");
			logo.init(logo_glunit, 0, 0, 20, 20);
			//

			font_texture = resource_dict::get_texture_handle(GL_RGBA, "assets/big_0.gif");
			//load all ship textures
			
			//file_reader::set_up_totals(num_guards,num_trap,num_diamond);

			background = resource_dict::get_texture_handle(GL_RGBA, "assets/diamonds/background.gif");
	
			thief = resource_dict::get_texture_handle(GL_RGBA, "assets/diamonds/thief_ab.gif");
			//r1 = rand() % 18 - 9, r2 = 9;
			thief_sprite.init(thief, 0, 0, 2, 2);

			empty_trap = resource_dict::get_texture_handle(GL_RGBA, "assets/diamonds/empty.gif");
			trap = resource_dict::get_texture_handle(GL_RGBA, "assets/diamonds/trap.gif");

			guard = resource_dict::get_texture_handle(GL_RGBA, "assets/diamonds/guard.gif");

			diamond = resource_dict::get_texture_handle(GL_RGBA, "assets/diamonds/diamond.gif");

			GameOver = resource_dict::get_texture_handle(GL_RGBA, "assets/diamonds/win.gif");

			GameFail = resource_dict::get_texture_handle(GL_RGBA, "assets/diamonds/lose.gif");

			borde_hor = resource_dict::get_texture_handle(GL_RGB, "assets/diamonds/brick2.gif");

			borde_vert = resource_dict::get_texture_handle(GL_RGB, "assets/diamonds/brick.gif");

			// sounds
			door = resource_dict::get_sound_handle(AL_FORMAT_MONO16, "assets/diamonds/door.wav");
			menu_music = resource_dict::get_sound_handle(AL_FORMAT_MONO16, "assets/diamonds/castle.wav");
			chase_music_file = resource_dict::get_sound_handle(AL_FORMAT_MONO16, "assets/diamonds/chase.wav");
			win_sound_file= resource_dict::get_sound_handle(AL_FORMAT_MONO16, "assets/diamonds/win.wav");
			cur_source = 0;
			alGenSources(num_sound_sources, sources);

			audio_inicio = get_sound_source();
			alSourcei(audio_inicio, AL_BUFFER, menu_music);
			alSourcePlay(audio_inicio);

			guard_velocity = 0.05f;

			game_start = false;
			game_over = false;
			game_fail = false;

		}

		// called every frame to move things
		void simulate() {
			 if (game_over) {
				return;
			}

			 if (game_fail) {
				 return;
			 }

			 if (!show_about_text && thief_sprite.check_collision(about_button)) {
				 about.translate(-50, 0);
				 show_about_text = true;
			  }

			
			 if (game_start) {

				 if (start_moving)
				 move_thief();

				 critical_interactions();

				 if (start_chasing) {
					 chasing();
					 switch_sounds();
				 }
				 else
				 {
					 move_guard(0, 0);
				 }

				 guard_collide(sprites[first_border_index + 2], sprites[first_border_index + 3]);


				 if (frames == 60)
				 {
					 turn_sprites_off();
					 start_moving = true;
				 }

				 frames++;
			 }

			 else
			 {
				 move_thief_intro();
				 go();
			 }
			
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

			start_button.render(texture_shader_, cameraToWorld);
			thief_sprite.render(texture_shader_, cameraToWorld);
			logo.render(texture_shader_, cameraToWorld);
			about.render(texture_shader_, cameraToWorld);
			about_button.render(texture_shader_, cameraToWorld);

			//char score_text[32];
			//sprintf(score_text, "score: %d   lives: %d\n", score, num_lives);
			//draw_text(texture_shader_, -1.75f, 2, 1.0f / 256, score_text);

			// move the listener with the camera
			vec4 &cpos = cameraToWorld.w();
			alListener3f(AL_POSITION, cpos.x(), cpos.y(), cpos.z());
		}
	};
}
