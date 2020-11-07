#include "main.h"
#include "mesh.h"


int main(int argc, char **argv)
{
	if(false == sphere_mesh.read_triangles_from_binary_stereo_lithography_file("sphere.stl"))
	{
		cout << "Error: Could not properly read file sphere.stl" << endl;
		return 2;
	}
	
	sphere_mesh.scale_mesh(2.0f);

	if (false == game_piece_mesh.read_triangles_from_binary_stereo_lithography_file("fractal.stl"))
	{
		cout << "Error: Could not properly read file fractal.stl" << endl;
		return 2;
	}

	game_piece_mesh.scale_mesh(0.5f);	

	vec3 dir(0, 0, 1);
	dir = normalize(dir);

	float yaw = 0.0f;

	if (fabsf(dir.x) < 0.00001 && fabsf(dir.z) < 0.00001)
		yaw = 0.0f;
	else
		yaw = atan2f(dir.x, dir.z);

	float pitch = -atan2f(dir.y, sqrt(dir.x * dir.x + dir.z * dir.z));

	game_piece_mesh.rotate_and_translate_mesh(yaw, pitch, dir);


	
	glutInit(&argc, argv);
	
	if (false == init_opengl(win_x, win_y))
		return 1;

	glutReshapeFunc(reshape_func);
	glutIdleFunc(idle_func);
	glutDisplayFunc(display_func);
	glutKeyboardFunc(keyboard_func);
	glutMouseFunc(mouse_func);
	glutMotionFunc(motion_func);
	glutPassiveMotionFunc(passive_motion_func);
	//glutIgnoreKeyRepeat(1);
	glutMainLoop();
	glutDestroyWindow(win_id);

	return 0;
}



void idle_func(void)
{
	glutPostRedisplay();
}

bool init_opengl(const int &width, const int &height)
{
	win_x = width;
	win_y = height;

	if(win_x < 1)
		win_x = 1;

	if(win_y < 1)
		win_y = 1;

	glutInitDisplayMode(GLUT_RGB|GLUT_DOUBLE|GLUT_DEPTH);
	glutInitWindowPosition(0, 0);
	glutInitWindowSize(win_x, win_y);
	win_id = glutCreateWindow("Binary Stereo Lithography file viewer");

	if (GLEW_OK != glewInit())
	{
		cout << "GLEW initialization error" << endl;
		return false;
	}

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_TRUE);
	glShadeModel(GL_SMOOTH);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	if (false == render.init("render.vs.glsl", "render.gs.glsl", "render.fs.glsl"))
	{
		cout << "Could not load render shader" << endl;
		return false;
	}

	uniforms.render.view_matrix = glGetUniformLocation(render.get_program(), "view_matrix");
	uniforms.render.proj_matrix = glGetUniformLocation(render.get_program(), "proj_matrix");
	uniforms.render.use_specular = glGetUniformLocation(render.get_program(), "use_specular");

	if (false == flat.init("flat.vs.glsl", "flat.fs.glsl"))
	{
		cout << "Could not load flat shader" << endl;
		return false;
	}

	uniforms.flat.view_matrix = glGetUniformLocation(flat.get_program(), "view_matrix");
	uniforms.flat.proj_matrix = glGetUniformLocation(flat.get_program(), "proj_matrix");
	uniforms.flat.flat_colour = glGetUniformLocation(flat.get_program(), "flat_colour");


	if (false == ssao.init("ssao.vs.glsl", "ssao.fs.glsl"))
	{
		cout << "Could not load SSAO shader" << endl;
		return false;
	}

	uniforms.ssao.ssao_radius = glGetUniformLocation(ssao.get_program(), "ssao_radius");
	uniforms.ssao.ssao_level = glGetUniformLocation(ssao.get_program(), "ssao_level");
	uniforms.ssao.object_level = glGetUniformLocation(ssao.get_program(), "object_level");
	uniforms.ssao.weight_by_angle = glGetUniformLocation(ssao.get_program(), "weight_by_angle");
	uniforms.ssao.randomize_points = glGetUniformLocation(ssao.get_program(), "randomize_points");
	uniforms.ssao.point_count = glGetUniformLocation(ssao.get_program(), "point_count");








	
	//For SSAO:


		ssao_level = 1.0f;
	ssao_radius = 0.05f;
	show_shading = true;
	show_ao = true;
	weight_by_angle = true;
	randomize_points = true;
	point_count = 100;


	glGenFramebuffers(1, &render_fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, render_fbo);
	glGenTextures(3, fbo_textures);

	glBindTexture(GL_TEXTURE_2D, fbo_textures[0]);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB16F, 2048, 2048);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glBindTexture(GL_TEXTURE_2D, fbo_textures[1]);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, 2048, 2048);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glBindTexture(GL_TEXTURE_2D, fbo_textures[2]);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT32F, 2048, 2048);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, fbo_textures[0], 0);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, fbo_textures[1], 0);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, fbo_textures[2], 0);

	static const GLenum draw_buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };

	glDrawBuffers(2, draw_buffers);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	SAMPLE_POINTS point_data;

	for (size_t i = 0; i < 256; i++)
	{
		do
		{
			point_data.point[i].x = random_float() * 2.0f - 1.0f;
			point_data.point[i].y = random_float() * 2.0f - 1.0f;
			point_data.point[i].z = random_float(); //  * 2.0f - 1.0f;
			point_data.point[i].w = 0.0f;
		}
		while (length(point_data.point[i]) > 1.0f);

		point_data.point[i] = normalize(point_data.point[i]);
	}

	for (size_t i = 0; i < 256; i++)
	{
		point_data.random_vectors[i].x = random_float();
		point_data.random_vectors[i].y = random_float();
		point_data.random_vectors[i].z = random_float();
		point_data.random_vectors[i].w = random_float();
	}

	glGenBuffers(1, &points_buffer);
	glBindBuffer(GL_UNIFORM_BUFFER, points_buffer);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(SAMPLE_POINTS), &point_data, GL_STATIC_DRAW);



	return true;
}

void reshape_func(int width, int height)
{
	win_x = width;
	win_y = height;

	if(win_x < 1)
		win_x = 1;

	if(win_y < 1)
		win_y = 1;

	glutSetWindow(win_id);
	glutReshapeWindow(win_x, win_y);
	glViewport(0, 0, win_x, win_y);

	main_camera.calculate_camera_matrices(win_x, win_y);
}



void draw_axis(void)
{
	unsigned int axis_buffer = 0;
	glGenBuffers(1, &axis_buffer);

	glLineWidth(2.0);

	glUseProgram(flat.get_program());

	main_camera.calculate_camera_matrices(win_x, win_y);
	glUniformMatrix4fv(uniforms.flat.proj_matrix, 1, GL_FALSE, &main_camera.projection_mat[0][0]);
	glUniformMatrix4fv(uniforms.flat.view_matrix, 1, GL_FALSE, &main_camera.view_mat[0][0]);

	const GLuint components_per_vertex = 3;
	const GLuint components_per_position = 3;

	vector<GLfloat> flat_data;

	flat_data.push_back(0);
	flat_data.push_back(0);
	flat_data.push_back(0);
	flat_data.push_back(2);
	flat_data.push_back(0);
	flat_data.push_back(0);

	glUniform3f(uniforms.flat.flat_colour, 1.0, 0.0, 0.0);

	GLuint num_vertices = static_cast<GLuint>(flat_data.size()) / components_per_vertex;

	glBindBuffer(GL_ARRAY_BUFFER, axis_buffer);
	glBufferData(GL_ARRAY_BUFFER, flat_data.size() * sizeof(GLfloat), &flat_data[0], GL_DYNAMIC_DRAW);

	glEnableVertexAttribArray(glGetAttribLocation(render.get_program(), "position"));
	glVertexAttribPointer(glGetAttribLocation(render.get_program(), "position"),
		components_per_position,
		GL_FLOAT,
		GL_FALSE,
		components_per_vertex * sizeof(GLfloat),
		NULL);

	glDrawArrays(GL_LINES, 0, num_vertices);

	flat_data.clear();

	flat_data.push_back(0);
	flat_data.push_back(0);
	flat_data.push_back(0);
	flat_data.push_back(0);
	flat_data.push_back(2);
	flat_data.push_back(0);

	glUniform3f(uniforms.flat.flat_colour, 0.0, 1.0, 0.0);

	//glDeleteBuffers(1, &axis_buffer);
	//glGenBuffers(1, &axis_buffer);

	num_vertices = static_cast<GLuint>(flat_data.size()) / components_per_vertex;

	glBindBuffer(GL_ARRAY_BUFFER, axis_buffer);
	glBufferData(GL_ARRAY_BUFFER, flat_data.size() * sizeof(GLfloat), &flat_data[0], GL_DYNAMIC_DRAW);

	glEnableVertexAttribArray(glGetAttribLocation(render.get_program(), "position"));
	glVertexAttribPointer(glGetAttribLocation(render.get_program(), "position"),
		components_per_position,
		GL_FLOAT,
		GL_FALSE,
		components_per_vertex * sizeof(GLfloat),
		NULL);

	glDrawArrays(GL_LINES, 0, num_vertices);

	flat_data.clear();

	flat_data.push_back(0);
	flat_data.push_back(0);
	flat_data.push_back(0);
	flat_data.push_back(0);
	flat_data.push_back(0);
	flat_data.push_back(2);

	glUniform3f(uniforms.flat.flat_colour, 0.0, 0.0, 1.0);

	//glDeleteBuffers(1, &axis_buffer);
	//glGenBuffers(1, &axis_buffer);

	num_vertices = static_cast<GLuint>(flat_data.size()) / components_per_vertex;

	glBindBuffer(GL_ARRAY_BUFFER, axis_buffer);
	glBufferData(GL_ARRAY_BUFFER, flat_data.size() * sizeof(GLfloat), &flat_data[0], GL_DYNAMIC_DRAW);

	glEnableVertexAttribArray(glGetAttribLocation(render.get_program(), "position"));
	glVertexAttribPointer(glGetAttribLocation(render.get_program(), "position"),
		components_per_position,
		GL_FLOAT,
		GL_FALSE,
		components_per_vertex * sizeof(GLfloat),
		NULL);

	glDrawArrays(GL_LINES, 0, num_vertices);

	glDeleteBuffers(1, &axis_buffer);
}

void draw_dot(void)
{
	glUseProgram(flat.get_program());

	glUniformMatrix4fv(uniforms.flat.proj_matrix, 1, GL_FALSE, &main_camera.projection_mat[0][0]);
	glUniformMatrix4fv(uniforms.flat.view_matrix, 1, GL_FALSE, &main_camera.view_mat[0][0]);
	glUniform3f(uniforms.flat.flat_colour, 1.0, 0.0, 0.0);

	GLint components_per_vertex = 3;
	GLint components_per_position = 3;

	glPointSize(4.0f);

	vector<float> flat_data;

	vec3 eye(main_camera.eye.x, main_camera.eye.y, main_camera.eye.z);

	vec3 pos = eye + ray;

	flat_data.push_back(pos.x);
	flat_data.push_back(pos.y);
	flat_data.push_back(pos.z);

	GLuint point_buffer;

	glGenBuffers(1, &point_buffer);

	GLint num_vertices = static_cast<GLuint>(flat_data.size()) / components_per_vertex;

	glBindBuffer(GL_ARRAY_BUFFER, point_buffer);
	glBufferData(GL_ARRAY_BUFFER, flat_data.size() * sizeof(GLfloat), &flat_data[0], GL_DYNAMIC_DRAW);

	glEnableVertexAttribArray(glGetAttribLocation(flat.get_program(), "position"));
	glVertexAttribPointer(glGetAttribLocation(flat.get_program(), "position"),
		components_per_position,
		GL_FLOAT,
		GL_FALSE,
		components_per_vertex * sizeof(GLfloat),
		NULL);

	glDrawArrays(GL_POINTS, 0, num_vertices);

	glDeleteBuffers(1, &point_buffer);
}





void draw_meshes(void)
{

	glUniform1i(uniforms.render.use_specular, 0);
	sphere_mesh.draw(render.get_program());

	glUniform1i(uniforms.render.use_specular, 1);
	game_piece_mesh.draw(render.get_program());
}



void display_func(void)
{
	/*
	Frustum lightFrustum;

	GLuint shadowFBO, pass1Index, pass2Index;

	int shadowMapWidth = 2048;
	int shadowMapHeight = 2048;

	mat4 lightPV, shadowBias;

	GLfloat border[] = { 1.0f, 0.0f,0.0f,0.0f };
	// The depth buffer texture
	GLuint depthTex;
	glGenTextures(1, &depthTex);
	glBindTexture(GL_TEXTURE_2D, depthTex);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT24, shadowMapWidth, shadowMapHeight);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LESS);

	// Assign the depth buffer usetexture to texture channel 0
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, depthTex);

	// Create and set up the FBO
	glGenFramebuffers(1, &shadowFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
		GL_TEXTURE_2D, depthTex, 0);

	GLenum drawBuffers[] = { GL_NONE };
	glDrawBuffers(1, drawBuffers);

	GLenum result = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (result == GL_FRAMEBUFFER_COMPLETE) {
		printf("Framebuffer is complete.\n");
	}
	else {
		printf("Framebuffer is not complete.\n");
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);


	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glEnable(GL_DEPTH_TEST);


	GLuint programHandle = shadow_map.get_program();
	pass1Index = glGetSubroutineIndex(programHandle, GL_FRAGMENT_SHADER, "recordDepth");
	pass2Index = glGetSubroutineIndex(programHandle, GL_FRAGMENT_SHADER, "shadeWithShadow");

	shadowBias = mat4(vec4(0.5f, 0.0f, 0.0f, 0.0f),
		vec4(0.0f, 0.5f, 0.0f, 0.0f),
		vec4(0.0f, 0.0f, 0.5f, 0.0f),
		vec4(0.5f, 0.5f, 0.5f, 1.0f)
	);

	vec3 lightPos = vec3(100.0f, 100.0f, 100.0f);  // World coords

	lightFrustum.orient(lightPos, vec3(0.0f), vec3(0.0f, 1.0f, 0.0f));
	lightFrustum.setPerspective(45.0f, 1.0f, 0.01f, 1000.0f);
	lightPV = shadowBias * lightFrustum.getProjectionMatrix() * lightFrustum.getViewMatrix();


	main_camera.calculate_camera_matrices(win_x, win_y);
	mat4 model(1.0f);
	mat4 mv = model * main_camera.view_mat;
	mat3 normal = mat3(vec3(mv[0]), vec3(mv[1]), vec3(mv[2]));
	mat4 mvp = main_camera.projection_mat * mv;
	mat4 shadow = lightPV * model;
	mat4 view = main_camera.view_mat;

	glUniformMatrix4fv(glGetUniformLocation(shadow_map.get_program(), "ModelViewMatrix"), 1, GL_FALSE, &mv[0][0]);
	glUniformMatrix3fv(glGetUniformLocation(shadow_map.get_program(), "NormalMatrix"), 1, GL_FALSE, &normal[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(shadow_map.get_program(), "MVP"), 1, GL_FALSE, &mvp[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(shadow_map.get_program(), "ShadowMatrix"), 1, GL_FALSE, &shadow[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(shadow_map.get_program(), "ViewMatrix"), 1, GL_FALSE, &view[0][0]);

	glUniform1i(glGetUniformLocation(shadow_map.get_program(), "shadow_map"), 0);






	shadow_map.use_program();


	mv = model * lightFrustum.getViewMatrix();
	normal = mat3(vec3(mv[0]), vec3(mv[1]), vec3(mv[2]));
	mvp = lightFrustum.getProjectionMatrix() * mv;
	lightPV = shadowBias * lightFrustum.getProjectionMatrix() * lightFrustum.getViewMatrix();
	shadow = lightPV * model;
	view = lightFrustum.getViewMatrix();

	glUniformMatrix4fv(glGetUniformLocation(shadow_map.get_program(), "ModelViewMatrix"), 1, GL_FALSE, &mv[0][0]);
	glUniformMatrix3fv(glGetUniformLocation(shadow_map.get_program(), "NormalMatrix"), 1, GL_FALSE, &normal[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(shadow_map.get_program(), "MVP"), 1, GL_FALSE, &mvp[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(shadow_map.get_program(), "ShadowMatrix"), 1, GL_FALSE, &shadow[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(shadow_map.get_program(), "ViewMatrix"), 1, GL_FALSE, &view[0][0]);

	glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
	glClear(GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, shadowMapWidth, shadowMapHeight);
	glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 1, &pass1Index);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	glEnable(GL_POLYGON_OFFSET_FILL);
	//	glPolygonOffset(2.5f, 10.0f);

	draw_meshes(shadow_map.get_program());
	glFlush();



	// reset camera matrices
	main_camera.calculate_camera_matrices(win_x, win_y);
	mv = model * main_camera.view_mat;
	normal = mat3(vec3(mv[0]), vec3(mv[1]), vec3(mv[2]));
	mvp = main_camera.projection_mat * mv;
	lightPV = shadowBias * lightFrustum.getProjectionMatrix() * lightFrustum.getViewMatrix();
	shadow = lightPV * model;
	view = main_camera.view_mat;

	glUniformMatrix4fv(glGetUniformLocation(shadow_map.get_program(), "ModelViewMatrix"), 1, GL_FALSE, &mv[0][0]);
	glUniformMatrix3fv(glGetUniformLocation(shadow_map.get_program(), "NormalMatrix"), 1, GL_FALSE, &normal[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(shadow_map.get_program(), "MVP"), 1, GL_FALSE, &mvp[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(shadow_map.get_program(), "ShadowMatrix"), 1, GL_FALSE, &shadow[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(shadow_map.get_program(), "ViewMatrix"), 1, GL_FALSE, &view[0][0]);

	glCullFace(GL_BACK);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, win_x, win_y);
	glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 1, &pass2Index);

	draw_meshes(shadow_map.get_program());




	glFlush();
	glutSwapBuffers();


	glDeleteTextures(1, &depthTex);
	glDeleteFramebuffers(1, &shadowFBO);

	*/





//For SSAO:

glEnable(GL_DEPTH_TEST);

	glUseProgram(render.get_program());

	const GLfloat background_colour[] = { 1.0f, 0.5f, 0.0f, 0.0f };
	static const GLfloat one = 1.0f;
	static const GLenum draw_buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };

	glBindFramebuffer(GL_FRAMEBUFFER, render_fbo);

	glEnable(GL_DEPTH_TEST);

	glClearBufferfv(GL_COLOR, 0, background_colour);
	glClearBufferfv(GL_COLOR, 1, background_colour);
	glClearBufferfv(GL_DEPTH, 0, &one);




	glBindBufferBase(GL_UNIFORM_BUFFER, 0, points_buffer);


	main_camera.calculate_camera_matrices(win_x, win_y);
	glUniformMatrix4fv(uniforms.render.proj_matrix, 1, GL_FALSE, &main_camera.projection_mat[0][0]);
	glUniformMatrix4fv(uniforms.render.view_matrix, 1, GL_FALSE, &main_camera.view_mat[0][0]);

	draw_meshes();

	draw_axis();


	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glUseProgram(ssao.get_program());

	glUniform1f(uniforms.ssao.ssao_radius, ssao_radius * float(win_x) / 1000.0f);
	glUniform1f(uniforms.ssao.ssao_level, show_ao ? (show_shading ? 0.3f : 1.0f) : 0.0f);
	glUniform1i(uniforms.ssao.weight_by_angle, weight_by_angle ? 1 : 0);
	glUniform1i(uniforms.ssao.randomize_points, randomize_points ? 1 : 0);
	glUniform1ui(uniforms.ssao.point_count, point_count);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, fbo_textures[0]);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, fbo_textures[1]);

	glGenVertexArrays(1, &quad_vao);

	glDisable(GL_DEPTH_TEST);
	glBindVertexArray(quad_vao);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glDeleteVertexArrays(1, &quad_vao);



	draw_dot();





	glFlush();
	glutSwapBuffers();




}

void keyboard_func(unsigned char key, int x, int y)
{
	switch(tolower(key))
	{
	case 'a':
		break;

	default:
		break;
	}
}

void mouse_func(int button, int state, int x, int y)
{
	if(GLUT_LEFT_BUTTON == button)
	{
		if (GLUT_DOWN == state)
		{
			ray = screen_coords_to_world_coords(x, y, win_x, win_y);

			lmb_down = true;
		}
		else
			lmb_down = false;
	}
	else if(GLUT_MIDDLE_BUTTON == button)
	{
		if(GLUT_DOWN == state)
			mmb_down = true;
		else
			mmb_down = false;
	}
	else if(GLUT_RIGHT_BUTTON == button)
	{
		if(GLUT_DOWN == state)
			rmb_down = true;
		else
			rmb_down = false;
	}
}

void motion_func(int x, int y)
{
	int prev_mouse_x = mouse_x;
	int prev_mouse_y = mouse_y;

	mouse_x = x;
	mouse_y = y;

	int mouse_delta_x = mouse_x - prev_mouse_x;
	int mouse_delta_y = prev_mouse_y - mouse_y;

	if(true == lmb_down && (0 != mouse_delta_x || 0 != mouse_delta_y))
	{
		//cout << main_camera.eye.x << ' ' << main_camera.eye.y << ' ' << main_camera.eye.z << endl;
		//cout << main_camera.look_at.x << ' ' << main_camera.look_at.y << ' ' << main_camera.look_at.z << endl;
		//cout << ray.x << ' ' << ray.y << ' ' << ray.z << endl;
		//cout << endl;

		main_camera.u -= static_cast<float>(mouse_delta_y)*u_spacer;
		main_camera.v += static_cast<float>(mouse_delta_x)*v_spacer;

		main_camera.calculate_camera_matrices(win_x, win_y);
	}
	else if(true == rmb_down && (0 != mouse_delta_y))
	{
		main_camera.w -= static_cast<float>(mouse_delta_y)*w_spacer;

		if(main_camera.w < 2.0f)
			main_camera.w = 2.0f;
		else if(main_camera.w > 20.0f)
			main_camera.w = 20.0f;

		main_camera.calculate_camera_matrices(win_x, win_y);
	}
}

void passive_motion_func(int x, int y)
{
	mouse_x = x;
	mouse_y = y;
}




