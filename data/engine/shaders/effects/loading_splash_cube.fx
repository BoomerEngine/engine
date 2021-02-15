/***
* Boomer Engine v4 2015-2017
* Written by Tomasz "Rex Dex" Jonarski
*
* Loading splash screen
*
***/

//--

descriptor SpashScreenParams
{
	ConstantBuffer
	{
		vec2 SourceRTSize;
		vec2 SourceInvRTSize;
		float Time;
		float Fade;
	}
}

export shader PS
{
	// https://www.shadertoy.com/view/3dScD1
	// Based on Hypercube VJ by BoyC
	// CC-SA

	const float edgeSize = 0.1;

	
	float vmax(vec3 v) {
		return max(max(v.x, v.y), v.z);
	}

	float vmax2(vec2 v) {
		return max(v.x, v.y);
	}
	
	float fBox2(vec2 p, vec2 b) {
		vec2 d = abs(p) - b;
		return length(max(d, vec2(0))) + vmax2(min(d, vec2(0)));
	}

	float fBox(vec3 p, vec3 b) {
		vec3 d = abs(p) - b;
		return length(max(d, vec3(0))) + vmax(min(d, vec3(0)));
	}

	float sdf(vec3 pos) {
		return fBox(pos, vec3(1.0,1.0,1.0));
	}

	float pMod1(out float p, float size) {
		float halfsize = size*0.5;
		float c = floor((p + halfsize)/size);
		p = mod(p + halfsize, size) - halfsize;
		return c;
	}

	vec2 pMod2(out vec2 p, vec2 size) {
		vec2 c = floor((p + size*0.5)/size);
		p = mod(p + size*0.5,size) - size*0.5;
		return c;
	}

	vec3 pMod3(out vec3 p, vec3 size) {
		vec3 c = floor((p + size*0.5)/size);
		p = mod(p + size*0.5, size) - size*0.5;
		return c;
	}

	float sdf2(vec3 pos) {
		pMod3(pos,vec3(2.0));
		return min(min(fBox2(pos.xy, vec2(edgeSize)),fBox2(pos.xz, vec2(edgeSize))),fBox2(pos.yz, vec2(edgeSize)));
	}

	vec2 pModMirror2(out vec2 p, vec2 size) {
		vec2 halfsize = size*0.5;
		vec2 c = floor((p + halfsize)/size);
		p = mod(p + halfsize, size) - halfsize;
		p *= mod(c,vec2(2))*2.0 - vec2(1);
		return c;
	}

	vec4 iqColor(vec3 a, vec3 b, vec3 c, vec3 d, float t) {
		return vec4( a + b * cos( 2.0 *3.14152965 * (c*t+d) ), 1);
	}

	vec4 discoStrip( float x ) {
		float patternLength = 30.0;
		float timePos = mod(Time,patternLength);
    
		float pattern = mod((Time / patternLength),6.0);
    
		float c = clamp(sin(Time*4.0 + x)*5.0,0.0,1.0);
		vec4 color = vec4(c*0.7,c*0.5,c,1);
    
		if (pattern>=1.0)
			color = iqColor(vec3(0.5),vec3(0.5),vec3(1.0),vec3(0.00, 0.33, 0.67) + vec3(x)/5.0, Time) * vec4(1.3,0.8,0.2,1.0);;
		if (pattern>=2.0)
			color = iqColor(vec3(0.5),vec3(0.5),vec3(1.0, 1.0, 0.5),vec3(0.80, 0.90, 0.30) + vec3(x)/6.0, Time);
		if (pattern>=3.0)
			color = vec4(sin(Time*5.0 + x)*0.5+0.5,cos(Time*5.0 + x)*0.7+0.3,cos(Time*7.0 + x)*0.3+0.5,1);
		if (pattern>=4.0)
			color = iqColor(vec3(0.5),vec3(0.5),vec3(1.0,1.0,0.5),vec3(0.8,0.90,0.30) + vec3(x)/3.0*1.5, Time) * vec4(0.3,0.8,1.5,1.0);
		if (pattern>=5.0)
			color = iqColor(vec3(0.5),vec3(0.5),vec3(1.0, 1.0, 0.5),vec3(0.80, 0.90, 0.30) + vec3(x)/6.0, Time) * vec4(0.7,1.5,0.3,1.0);/**/
    
		float fade = clamp(-(abs(timePos-patternLength/2.0)-patternLength/2.0)-0.5,0.0,1.0);        
		return color*fade;
	}

	vec4 dotPattern(vec2 d) {
		d -= vec2(11.0);
		
		d=mod(d,40.0);
		if (d.x>=20.0) d.x=39.0-d.x;
		if (d.y>=20.0) d.y=39.0-d.y;
		
		if (d.x-0.0 == 0.0 )
			return discoStrip( d.y );
		if (d.x-19.0 == 0.0 )
			return discoStrip( d.y );
		
		if (d.y-0.0 == 0.0 )
			return discoStrip( d.x );
		if (d.y-19.0 == 0.0 )
			return discoStrip( d.x );
		
		return vec4(0.0);
	}

	vec4 discoPattern(vec2 pos) {
		pos+=vec2(edgeSize*0.5);
		vec2 idx = pMod2(pos,vec2(edgeSize*1.0));
		float l = length(pos)*3.0;
    
		vec4 dotColor = clamp(dotPattern(idx),vec4(0.0),vec4(1.0));    
		return l<edgeSize ? dotColor : vec4(0);
	}

	vec4 disco(vec3 pos, vec3 dir) {
		vec3 n=normalize(vec3(sdf2(pos+vec3(0.01,0,0))-sdf2(pos+vec3(-0.01,0,0)),
							sdf2(pos+vec3(0,0.01,0))-sdf2(pos+vec3(0,-0.01,0)),
							sdf2(pos+vec3(0,0,0.01))-sdf2(pos+vec3(0,0,-0.01))));

		n=abs(n);
    
		vec4 c = vec4(0);
		if (n.x > n.y && n.x > n.z)
			c=discoPattern(pos.yz);
		if (n.y > n.x && n.y > n.z)
			c=discoPattern(pos.xz);
		if (n.z > n.y && n.z > n.x)
			c=discoPattern(pos.xy);
    
		return c;
	}

	vec4 magicCube(vec3 pos, vec3 dir) {
		vec3 oPos = pos;

		float dist = sdf2(pos+1.0);
		if (dist<0.000001)
			return vec4(0);
	  
		for (int x=0; x<100; x+=1) {
			dist = sdf2(pos+1.0);
		  
			float distFade = max(0.0, 1.0-length(pos-oPos)*0.05 );
			if (distFade==0.0)
				break;
		  
			if (dist<0.00001)
				return lerp(disco(pos, dir),vec4(1.0),1.0-distFade);
			  
			pos += dir * abs(dist);       
		}
		
		return vec4(1.0);
	}

	void main()
    {
		ivec2 sourcePixel = gl_FragCoord.xy;
		vec2 sourceUV = (sourcePixel + 0.5f) * SourceInvRTSize;
		
		float time = SpashScreenParams.Time / 3.0;//+43.63/3.0;

		// cam that goes in the cube
		/*float rotationTime = sin(time/5.35886)*3.14152965;
		float y = 1.0*sin(time + sin(time/5.0)*3.14152965);
		vec3 Pos=vec3(2.0*sin(rotationTime), y, 2.0*cos(rotationTime))*(1.3+sin(time)*( sin(time/7.86463) * 0.5 + 0.5 ) );/**/
    
		// cam that doesn't go in the cube
		float rotationTime = sin(time/5.35886)*3.14152965;
		float y = 1.0*sin(time + sin(time/5.0)*3.14152965);
		vec3 Pos=vec3(2.0*sin(rotationTime), y, 2.0*cos(rotationTime))*(2.0+sin(time)*( sin(time/7.86463) * 0.1 + 0.1 ) );
    
		vec3 camPos = Pos;
		vec3 CamDir=normalize(-Pos);
		vec3 CamRight=normalize(cross(CamDir,vec3(0,1,0)));
		vec3 CamUp=normalize(cross(CamRight,CamDir));
		mat3 cam = mat3(CamRight,CamUp,CamDir);

		//ray calculation	
		float distroPower = clamp((sin(time/2.1373)-0.95)*20.0,0.0,1.0);
		vec2 uv = 2.0 * sourceUV - 1.0;
		float aspect = SourceRTSize.x / SourceRTSize.y;
	
		vec3 Dir = cam * normalize(vec3(uv * vec2(aspect, 1.0), 2.0));
		float dist = 10000.0f;

		for (int x=0; x<100; x+=1)
		{
			dist = sdf(Pos);
			if (dist<=0.01)
				break;
			Pos += Dir * abs(dist);        
		}
    
		if (dist<=0.01)
		{
			float camDist = length(camPos);
			gl_Target0 = magicCube(Pos,Dir).xyz1;
		}
		else
		{
			float vignette = clamp(1.0 - pow(length( (sourceUV - 0.5) * 2.0 ),5.0),0.0,1.0);
			gl_Target0 = ((discoStrip(Dir.x*5.0) + discoStrip(Dir.y*5.0) + discoStrip(Dir.z*5.0))/3.0*0.5 * vignette*0.7).xyz1;
		}

		//gl_Target0 = pow((gl_Target0 * Fade).xyz1, 1.0 / 2.2);
		gl_Target0 = (gl_Target0 * Fade).xyz1;
	}
}

export shader VS
{
	void main()
	{
		gl_Position.x = (gl_VertexID & 1) ? -1.0f : 1.0f;
		gl_Position.y = (gl_VertexID & 2) ? -1.0f : 1.0f;	
		gl_Position.z = 0.5f;
		gl_Position.w = 1.0f;
	}
}

//----
