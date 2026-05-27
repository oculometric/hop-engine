#pragma CANVAS_ATTACHMENTS
#pragma CANVAS_TRANSFORM

vec3 computeBarrelDistortedPosition(vec2 centered_uv, float strength, float square_fraction, float mask_width)
{
	float d = length(centered_uv);
	vec2 distorted_uv = (centered_uv * (1.0f + (d * d * square_fraction * strength) + (d * d * d * d * (1.0f - square_fraction) * strength))) / (1.0f + (strength * 0.85f));

	vec2 mask2 = clamp((abs(distorted_uv) - (1.0f - mask_width)) / mask_width, 0, 1);
	float mask = (mask2.x + mask2.y) - (abs(mask2.x) * abs(mask2.y));

	return vec3(distorted_uv, mask);
}

uniform sampler2D camera;

bool fragment(in Varyings vars, inout Fragment frag)
{
	vec3 barrel_uv = computeBarrelDistortedPosition(vars.uv.xy * 2.0f - 1.0f, 0.1f, 0.8f, 0.04f);
	vec2 distorted_uv = (barrel_uv.xy + 1.0f) / 2.0f;

	if (barrel_uv.z > 0)
		return false;//frag.colour = vec4(0, 0, 0, 0);//vec4(pow(texture(backing_tex, vec2(vars.uv.x, 1.0f - vars.uv.y)).rgb, vec3(1.0f / 2.2f)), 1);
	else if (abs(barrel_uv.x) > 0.94f || abs(barrel_uv.y) > 0.94f)
        frag.colour = vec4(0.02f, 0.02f, 0.02f, 1.0f);
    else
	{
		vec3 colour = texture(camera, distorted_uv).rgb;
		// very subtle chromatic aberration
		vec2 uv_offset = 2.0f / vec2(textureSize(camera, 0));
		vec3 left_colour = texture(camera, distorted_uv + vec2(uv_offset.x, 0)).rgb;
		vec3 right_colour = texture(camera, distorted_uv - vec2(uv_offset.x, 0)).rgb;
		colour = (0.8f * colour) + (0.1f * left_colour * vec3(2, 0, 0)) + (0.1f * right_colour * vec3(0, 0, 2));

		// reduce contrast
		const float contrast = 0.98f;
		colour = 1.0f - ((1.0f - colour) * contrast);

		// TODO: rgb noise

		// line distortions
		float line = barrel_uv.y * 92.0f;
		if ((round(line) - line) < 0.0f)
			colour *= vec3(0.9f, 0.95f, 0.9f);

		frag.colour = vec4(colour, 1);
	}
    return true;
}