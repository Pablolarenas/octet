////////////////////////////////////////////////////////////////////////////////
//
// (C) Andy Thomason 2012-2013
//
// Modular Framework for OpenGLES2 rendering on multiple platforms.
//
// animation example: Drawing an jointed figure with animation
//
// Level: 2
//
// Demonstrates:
//   Collada meshes
//   Collada nodes
//   Collada animation
//

class animation_app : public app {
  scene app_scene;

  // shader to draw a shaded, textured triangle
  bump_shader shader_;
public:

  // this is called when we construct the class
  animation_app(int argc, char **argv) : app(argc, argv) {
  }

  // this is called once OpenGL is initialized
  void app_init() {
    // set up the shader
    shader_.init();

    collada_builder builder;
    //builder.load("assets/blender.freemovies.co.uk/monkeyManRunCycleTutFinal.dae");
    //builder.load("assets/opengameart.org/KzuOpenGameArt_3uhox/FemaleOpenGameArt.dae");
    builder.load("assets/skinning/skin.dae");
//C:\projects\octet\octet\assets\opengameart.org\KzuOpenGameArt_3uhox\FemaleOpenGameArt.dae
    //builder.load("assets/cubeattr.dae");
    //builder.load("assets/box_cone.dae");

    const char *def_scene = builder.get_default_scene();
    app_scene.make_collada_scene(builder, def_scene);
  }

  // this is called to draw the world
  void draw_world(int x, int y, int w, int h) {
    // set a viewport - includes whole window area
    glViewport(x, y, w, h);

    // clear the background to black
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // allow Z buffer depth testing (closer objects are always drawn in front of far ones)
    glEnable(GL_DEPTH_TEST);

    // improve draw speed by culling back faces - and avoid flickering edges
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    app_scene.render(shader_);
  }
};
