#version 150

in vec3 position;
in vec4 color;
out vec4 col;

uniform int mode;

// For mode
in vec3 PleftPosition, PrightPosition, PdownPosition, PupPosition;

uniform mat4 modelViewMatrix;
uniform mat4 projectionMatrix;

void main()
{
  // compute the transformed and projected vertex position (into gl_Position) 
  // compute the vertex color (into col)

  if(mode == 1)
  {
  	float smoothenedHeight;
  	vec4 tempColor;
  	smoothenedHeight = float(PleftPosition.y + PrightPosition.y + PdownPosition.y + PupPosition.y) / 4.0f;
  	tempColor = smoothenedHeight * max(color, vec4(0.0000001f)) / max(position.y, 0.000004f);
  	tempColor.a = 1.0f;
  	gl_Position = projectionMatrix * modelViewMatrix * vec4(position.x, smoothenedHeight, position.z, 1.0f);
  	col = tempColor;
  }
  else if(mode == 0)
  {
  	gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0f);
  	col = color;
  }
}

