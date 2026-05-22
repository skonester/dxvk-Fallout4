#version 450

#extension GL_GOOGLE_include_directive : require

#include "hud_frag_common.glsl"

layout(location = 0) in  vec2 v_coord;
layout(location = 0) out vec4 o_color;

#define NUM_DATA_POINTS (300)

layout(binding = 0, std430)
readonly buffer data_buffer_t {
  float simd_us[NUM_DATA_POINTS];
};

layout(push_constant)
uniform push_data_t {
  uvec2 surface_size;
  float opacity;
  float scale;
  uint  samplerIndex;
  uint  packed_xy;
  uint  packed_wh;
  uint  frame_index;
};

int max_index = NUM_DATA_POINTS - 1;

int compute_real_index(int local_index) {
  int real_index = int(frame_index) - local_index;

  if (real_index < 0)
    real_index += NUM_DATA_POINTS;

  return real_index;
}

float sample_at(float position) {
  int local_index = int(position);

  int lo_index = compute_real_index(clamp(local_index, 0, max_index));
  int hi_index = compute_real_index(clamp(local_index + 1, 0, max_index));

  float lo_value = simd_us[lo_index];
  float hi_value = simd_us[hi_index];

  return mix(lo_value, hi_value, fract(position));
}

void main() {
  float x_pos = (1.0f - v_coord.x) * float(NUM_DATA_POINTS);

  float x_delta = abs(dFdx(x_pos)) / 2.0f;
  float y_delta = abs(dFdy(v_coord.y));

  float us_l = sample_at(x_pos - x_delta);
  float us_r = sample_at(x_pos + x_delta);

  float us_lo = min(us_l, us_r);
  float us_hi = max(us_l, us_r);

  // Auto-scale based on running max, with reasonable defaults
  float us_max = 100.0f; // Default max 100 µs
  for (int i = 0; i < NUM_DATA_POINTS; i++) {
    us_max = max(us_max, simd_us[i]);
  }
  us_max = max(us_max, 50.0f); // At least 50 µs visible

  float val_lo = min(us_lo / us_max, 1.0f) - y_delta;
  float val_hi = min(us_hi / us_max, 1.0f) - y_delta;

  float diff_lo = min(v_coord.y - val_lo, 0.0f);
  float diff_hi = min(val_hi - v_coord.y, 0.0f);

  vec4 line_color = vec4(0.4f, 0.9f, 0.8f, 1.0f); // Cyan/teal
  vec4 bg_color = vec4(0.0f, 0.0f, 0.0f, 0.0f);

  // Try to draw a somewhat defined line
  float diff = (diff_lo + diff_hi) + y_delta;
  o_color = mix(bg_color, line_color, clamp(diff / y_delta, 0.0f, 1.0f));

  o_color.a *= opacity;
  o_color.rgb *= o_color.a;

  o_color = linear_to_output(o_color);
}