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
		
	
	};

	class invaderers_app : public octet::app {
		// Matrix to transform points in our camera space to the world.
		// This lets us move our camera
		mat4t cameraToWorld;

		// shader to draw a textured triangle
		texture_shader texture_shader_;

		// game state
		bool game_over;
		bool game_fail;
		bool start_chasing=false;


		// sounds
		ALuint whoosh;
		ALuint bang;
		unsigned cur_source;
		//ALuint sources[num_sound_sources];

		// big array of sprites
		sprite sprites[300];

		// number of guards and traps (blue diamonds)
		enum {
			num_guards = 6,
			num_trap = 3,
			num_diamond = 1,

			win_sprite,
			lose_sprite,
		};

		//////////////// que es esto pedro?? ////////////////
		int current_sprite;
		int thief_sprite_index;
		int first_guard_sprite_index;
		int first_trap_sprite_index;
		int first_diamond_sprite_index;

		// random number generator
		class random randomizer;

		// a texture for our text
		GLuint font_texture;

		// information for our text
		bitmap_font font;

		//ALuint get_sound_source() { return sources[cur_source++ % num_sound_sources]; }

		// use the keyboard to move the thief
		void move_thief() {
			const float ship_speed = 0.3f;

			// left and right arrows
			if (is_key_down(key_left)) {
				sprites[thief_sprite_index].translate(-ship_speed, 0);	
			}
			else if (is_key_down(key_right)) {
				sprites[thief_sprite_index].translate(+ship_speed, 0);
			}
			else if (is_key_down(key_up)) {
				sprites[thief_sprite_index].translate(0, +ship_speed);
			}
			else if (is_key_down(key_down)) {
				sprites[thief_sprite_index].translate(0, -ship_speed);
			}
			
		}

		void critical_interactions () {

			// if our thief collides with the true diamond! 
			for (int j = 0; j < num_diamond; j++) {
				game_over= sprites[thief_sprite_index].check_collision(sprites[first_diamond_sprite_index + j]);
				if (game_over)
				{
					printf("you win!");
					sprites[win_sprite].translate(-20, 0);
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

			// if our thief collides with a guard 
			for (int j = 0; j < num_guards; j++) {
				game_fail = sprites[thief_sprite_index].check_collision(sprites[first_guard_sprite_index + j]);
				if (game_fail)
				{
					printf("you lose");
					sprites[lose_sprite].translate(-20, 0);
					return;
				}
			
			}

		}

		// how guards move in a chasing
		void chasing() {
			for (int j = 1; j <= num_guards; j++) {
				float movement_x = 0.025f;
				float movement_y = 0.025f;
				if (sprites[thief_sprite_index].x < sprites[first_guard_sprite_index].x)
					movement_x = movement_x * -1;
				if (sprites[thief_sprite_index].y < sprites[first_guard_sprite_index].y)
					movement_y = movement_y * -1;
				sprites[first_guard_sprite_index].translate(movement_x, movement_y);
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

			current_sprite = 0;
			srand(time(NULL));

			GLuint thief = resource_dict::get_texture_handle(GL_RGBA, "assets/diamonds/thief.gif");
			thief_sprite_index = current_sprite;
			sprites[current_sprite++].init(thief, 0, 0, 1, 1);

			GLuint guard = resource_dict::get_texture_handle(GL_RGBA, "assets/diamonds/guard.gif");
			first_guard_sprite_index = current_sprite;
			for (int j=0;j<num_guards; j++) {
				int r1 = rand() % 20 - 10,
					r2 = rand() % 20 - 10;
			sprites[current_sprite++].init(guard, r1, r2, 1, 1);
			}

			GLuint diamond = resource_dict::get_texture_handle(GL_RGBA, "assets/diamonds/diamond.gif");
			first_diamond_sprite_index = current_sprite;
			for (int j = 0; j<num_diamond; j++) {
				int r1 = rand() % 20 - 10,
					r2 = rand() % 20 - 10;
				sprites[current_sprite++].init(diamond, r1, r2, 1, 1);
			}

			GLuint trap = resource_dict::get_texture_handle(GL_RGBA, "assets/diamonds/trap.gif");
			first_trap_sprite_index = current_sprite;
			for (int j = 0; j<num_trap; j++) {
				int r1 = rand() % 20 - 10,
					r2 = rand() % 20 - 10;
				sprites[current_sprite++].init(trap, r1, r2, 1, 1);
			}

			GLuint GameOver = resource_dict::get_texture_handle(GL_RGBA, "assets/diamonds/win.gif");
			sprites[win_sprite].init(GameOver, 20, 0, 3, 1.5f);

			GLuint GameFail = resource_dict::get_texture_handle(GL_RGBA, "assets/diamonds/lose.gif");
			sprites[lose_sprite].init(GameFail, 20, 0, 3, 1.5f);


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

			move_thief();
			critical_interactions();

			if (start_chasing) {
				chasing();
			}

		}

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
			for (int i = 0; i != current_sprite; ++i) {
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
