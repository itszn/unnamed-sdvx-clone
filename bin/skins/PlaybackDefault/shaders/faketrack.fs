#extension GL_ARB_separate_shader_objects : enable
layout(location=1) in vec2 fsTex;
layout(location=0) out vec4 target;

uniform sampler2D trackTex;
uniform sampler2D buttonTex;
uniform sampler2D data_0;
uniform sampler2D data_1;
uniform sampler2D data_2;
uniform sampler2D data_3;

uniform ivec2 viewport;
uniform float songtime;
uniform float bpm;
uniform float num_beats;

uniform vec4 color;

// Render one color over the other if the second is non transparent
vec4 render_over(vec4 col1, vec4 col2) {
  if (col2.a > 0)
    return col2;
  return col1;
}

// Draw a button at a given location using bounds checking
vec4 draw_button(vec2 track_rel, vec2 loc) {

  // Height has to scale as we move down the track, I did a simple linear scale since
  // the track itself is a linear scale
  vec2 button_dim = vec2(
    1.0/6.0 - .03, // Button width
    .50);
  loc.x += .015; // Some padding

  vec2 btn_rel = track_rel - loc + vec2(0, button_dim.y/2);

  // Check if we are in the button
  if (
      btn_rel.x < 0 || btn_rel.x > button_dim.x || 
      btn_rel.y < 0 || btn_rel.y > button_dim.y) {
    return vec4(0,0,0,0);
  } 


  vec2 uv = vec2(btn_rel.x / button_dim.x, .5);
  vec4 col = texture(buttonTex, uv);
  return col;
}

//float bpm = 250;
float nps = bpm * 4 / 60;
//float num_beats = 196.0f;

float buttons_on_screen = 15.0f;
float button_space_multi = buttons_on_screen / num_beats;

vec4 render_button_from_data(sampler2D data_tex, vec2 track_rel, float x_pos) {
  vec2 rel = track_rel;
  float track_y = rel.y;

  // This is our starting time for the current data
  float time_off = songtime - 5.68720379147 + 0.05;

  // Calculate where the current time maps into the data
  time_off = (time_off*nps)/num_beats;

  // This is equivalent to the "speed", but basically means the space between notes or number of notes on screen
  float speed = button_space_multi;

  // data_y says how far into the track data to go
  // this will be the current time offset + some function to get the position on track

  float data_y = time_off + speed * track_rel.y;

  // Cut off after or before the data starts
  
  if (data_y < 0 || data_y > 1) {
    return vec4(0,0,0,0);
  }
  

  // We grab a datapoint from the texture (y is the distance into the song)
  vec4 data_point = texture(data_tex, vec2(.5, data_y));

  //return data_point;

  // Since this is a shader, we will get interpolated data so we don't have a binary
  // "note or not note" from that, but we can use the linear interpolation to calculate
  // how far from the note, so we can calculate where on the screen the note is, and then
  // perform bounds checking to draw it.

  // If x < 1 then there is maybe note near by and we can use x to find its location
  // y tells us if we are above or below the note


  float fun = x_pos;
  /*
  if (x_pos >= 3.0/6.0) {
    fun = x_pos - 3.0/6.0*(track_rel.y);
  } else {
    fun = x_pos + 3.0/6.0*(track_rel.y);
  }
  */

  vec2 loc = vec2(fun, track_rel.y);
  if (data_point.y == 0) {
    loc.y = loc.y + data_point.x;
  } else {
    loc.y = loc.y - data_point.x;
  }

  // If we are near the note, try to render it via bounds checking
  if (data_point.x < 1.0f) {
      return
      //render_over(data_point, // Debug: uncomment this line and one below to render data
        draw_button(rel, loc)
      //)
      ;
  }

  //return data_point; // Debug: uncomment this line to render data

  return vec4(0,0,0,0);
}

void main()
{
    vec4 col = texture(trackTex, fsTex);
    vec4 mainColor = col;

    // Color the sides of the lane
    col.xyz = vec3(.9) * vec3(0,170/255,1) * vec3(mainColor.z);
    col.xyz += vec3(.9) * vec3(1, 0,128/255) * vec3(mainColor.x);
    col.xyz += vec3(.6) * vec3(mainColor.y);

    col = render_over(col,
        render_button_from_data(data_0, fsTex, 1.0/6.0)
    );
    col = render_over(col,
        render_button_from_data(data_1, fsTex, 2.0/6.0)
    );
    col = render_over(col,
        render_button_from_data(data_2, fsTex, 3.0/6.0)
    );
    col = render_over(col,
        render_button_from_data(data_3, fsTex, 4.0/6.0)
    );

    target = col;

}