
layout(triangles_adjacency) in;

#ifdef DEBUG
layout(line_strip, max_vertices=18) out;
#else
layout(triangle_strip, max_vertices=18) out;
#endif

uniform mat4 u_projMat;
uniform vec4 u_lightPos;  // Light position (eye space)

//uniform int robust;  // Robust generation needed?
//uniform int zpass;  // Is it safe to do z-pass?

// float EPSILON = 0.01;
int robust=0;
int zpass=0;

void main() {


  vec3 ns[3];  // Normals
  vec3 d[3];  // Directions toward light
  vec4 v[4];  // Temporary vertices

  //Triangle oriented toward light source
  vec4 or_pos[3] = {
    gl_in[0].gl_Position,
    gl_in[2].gl_Position,
    gl_in[4].gl_Position
  };

  // Compute normal at each vertex.
  ns[0] = cross(gl_in[2].gl_Position.xyz - gl_in[0].gl_Position.xyz,
                gl_in[4].gl_Position.xyz - gl_in[0].gl_Position.xyz );
  ns[1] = cross(gl_in[4].gl_Position.xyz - gl_in[2].gl_Position.xyz,
                gl_in[0].gl_Position.xyz - gl_in[2].gl_Position.xyz );
  ns[2] = cross(gl_in[0].gl_Position.xyz - gl_in[4].gl_Position.xyz,
                gl_in[2].gl_Position.xyz - gl_in[4].gl_Position.xyz );

  // Compute direction from vertices to light.
  d[0] = u_lightPos.xyz-u_lightPos.w*gl_in[0].gl_Position.xyz;
  d[1] = u_lightPos.xyz-u_lightPos.w*gl_in[2].gl_Position.xyz;
  d[2] = u_lightPos.xyz-u_lightPos.w*gl_in[4].gl_Position.xyz;


  // Check if the main triangle faces the light.
  bool faces_light = true;

  if(!(dot(ns[0],d[0])>0.0 || dot(ns[1],d[1])>0.0 || dot(ns[2],d[2])>0.0)) {
    // Not facing the light and not robust, ignore.
    if(robust == 0) {
      return;
    }

    // Flip vertex winding order in or_pos.
    or_pos[1] = gl_in[4].gl_Position;
    or_pos[2] = gl_in[2].gl_Position;
    faces_light = false;
  }

  // Render caps. This is only needed for z-fail.
  if(zpass == 0) {
    // Near cap: simply render triangle.
    gl_Position = u_projMat*or_pos[0];
    // gl_Position.z+=EPSILON;
    EmitVertex();
    gl_Position = u_projMat*or_pos[1];
    // gl_Position.z+=EPSILON;
    EmitVertex();
    gl_Position = u_projMat*or_pos[2];
    // gl_Position.z+=EPSILON;
    EmitVertex();
    EndPrimitive();

    // Far cap: extrude positions to infinity.
    v[0] =vec4(u_lightPos.w*or_pos[0].xyz-u_lightPos.xyz,0);
    v[1] =vec4(u_lightPos.w*or_pos[2].xyz-u_lightPos.xyz,0);
    v[2] =vec4(u_lightPos.w*or_pos[1].xyz-u_lightPos.xyz,0);
    gl_Position = u_projMat*v[0];
    // gl_Position.z+=EPSILON;
    EmitVertex();
    gl_Position = u_projMat*v[1];
    // gl_Position.z+=EPSILON;
    EmitVertex();
    gl_Position = u_projMat*v[2];
    // gl_Position.z+=EPSILON;
    EmitVertex();
    EndPrimitive();
  }

  // Loop over all edges and extrude if needed.
  for(int i=0; i<3; i++) {
    // Compute indices of neighbor triangle.
    int v0 = i*2;
    int nb = (i*2+1);
    int v1 = (i*2+2) % 6;

    // Compute normals at vertices, the *exact*
    // same way as done above!
    ns[0] = cross(gl_in[nb].gl_Position.xyz-gl_in[v0].gl_Position.xyz,
                  gl_in[v1].gl_Position.xyz-gl_in[v0].gl_Position.xyz);
    ns[1] = cross(gl_in[v1].gl_Position.xyz-gl_in[nb].gl_Position.xyz,
                  gl_in[v0].gl_Position.xyz-gl_in[nb].gl_Position.xyz);
    ns[2] = cross(gl_in[v0].gl_Position.xyz-gl_in[v1].gl_Position.xyz,
                  gl_in[nb].gl_Position.xyz-gl_in[v1].gl_Position.xyz);

    // Compute direction to light, again as above.
    d[0]=u_lightPos.xyz-u_lightPos.w*gl_in[v0].gl_Position.xyz;
    d[1]=u_lightPos.xyz-u_lightPos.w*gl_in[nb].gl_Position.xyz;
    d[2]=u_lightPos.xyz-u_lightPos.w*gl_in[v1].gl_Position.xyz;


    // Extrude the edge if it does not have a
    // neighbor, or if it's a possible silhouette.
    if(gl_in[nb].gl_Position.w < 1e-3 ||
       (faces_light != (dot(ns[0],d[0])>0 ||
                        dot(ns[1],d[1])>0 ||
                        dot(ns[2],d[2])>0) ))
      {
        // Make sure sides are oriented correctly.
        int i0 = faces_light ? v0 : v1;
        int i1 = faces_light ? v1 : v0;

        v[0] = gl_in[i0].gl_Position;
        v[1] = vec4(u_lightPos.w*gl_in[i0].gl_Position.xyz - u_lightPos.xyz, 0.0);
        v[2] = gl_in[i1].gl_Position;
        v[3] = vec4(u_lightPos.w*gl_in[i1].gl_Position.xyz - u_lightPos.xyz, 0.0);

        // Emit a quad as a triangle strip.
        gl_Position = u_projMat*v[0];
        // gl_Position.z+=EPSILON;
        EmitVertex();
        gl_Position = u_projMat*v[1];
        // gl_Position.z+=EPSILON;
        EmitVertex();
        gl_Position = u_projMat*v[2];
        // gl_Position.z+=EPSILON;
        EmitVertex();
        gl_Position = u_projMat*v[3];
        // gl_Position.z+=EPSILON;
        EmitVertex();
        EndPrimitive();
      }
  }
}
