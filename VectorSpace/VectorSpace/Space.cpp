/*
Object for a general volume of space, used for containing objects and rendering
*/
#include <string>
#include <iostream>
#include "Space.h"
#include "Helpers.h"

// Factors to calculate depth effect
const float DEPTH_SCALE_FACTOR = 0.34f;
const float DEPTH_OFFSET_FACTOR = 0.3f;

const float CAMERA_TURN_SPEED = 1.5f;
const float ZOOM_SPEED = 100.f;

const bool SHOW_FRAME_COUNTER = true;

const std::string NODE_IMAGE = "images\\node_item.png";

Space::Space()
{
	initTime();
	loadImages();

	Node* god = new Node(12, 0, 0);
	god->m_color = new sf::Color(255, 0, 0);
	AddNode(god);

	if (true) return;

	// For debugging purposes : test node
	Node* testnode = new Node(12, 0, 0);
	testnode->m_color = new sf::Color(255, 0, 0);
	AddNode(testnode);

	Node* testchild = new Node(10, 200, 0);
	AddChild(testnode, testchild);

	// Debug : square of nodes
	Node* a = new Node(8, 400, 400, 400);
	a->m_color = new sf::Color(0, 0, 255);
	Node* b = new Node(8, -400, 400, 400);
	b->m_color = new sf::Color(255, 255, 0);
	Node* c = new Node(8, 400, 400, -400);
	c->m_color = new sf::Color(0, 255, 0);
	Node* d = new Node(8, -400, 400, -400);
	d->m_color = new sf::Color(180, 0, 220);
	Node* e = new Node(8, 400, -400, 400);
	e->m_color = new sf::Color(255, 150, 0);
	Node* f = new Node(8, -400, -400, 400);
	f->m_color = new sf::Color(100, 100, 100);
	Node* g = new Node(8, 400, -400, -400);
	g->m_color = new sf::Color(255, 0, 255);
	Node* h = new Node(8, -400, -400, -400);
	m_nodes.push_back(a);
	m_nodes.push_back(b);
	m_nodes.push_back(c);
	m_nodes.push_back(d);
	m_nodes.push_back(e);
	m_nodes.push_back(f);
	m_nodes.push_back(g);
	m_nodes.push_back(h);
	// Debug : test links
	Link* testlink = new Link(testnode, e);
	Link* la = new Link(a, g);
	Link* lb = new Link(b, d);
	Link* lc = new Link(c, b);
	Link* ld = new Link(f, e);
	Link* le = new Link(h, b);
	Link* lf = new Link(f, a);
	m_links.push_back(testlink);
	m_links.push_back(la);
	m_links.push_back(lb);
	m_links.push_back(lc);
	m_links.push_back(ld);
	m_links.push_back(le);
	m_links.push_back(lf);
}

Space::~Space()
{
}

void Space::Tick() // Main tick function of space : called every update
{
	UpdateTime();
	Render();
	HandleInput();
	ProcessItems();
	if (SHOW_FRAME_COUNTER) UpdateFrameTimer();
}

void Space::initTime()
{
	runtimeClock.restart();
	deltaClock.restart();
	printf("Space: Time initialized\n");
}

void Space::loadImages()
{
	// Load image for node object
	if (!t_node.loadFromFile(NODE_IMAGE))
		printf("Could not load node texture!");
	t_node.setSmooth(true);

	t_node_size = (float)t_node.getSize().x;
}

void Space::SetWindow(sf::RenderWindow* new_window)
{
	window = new_window;

	// Set window specifications
	sf::Vector2u window_size = window->getSize();
	w_height = (float)window_size.y;
	w_width = (float)window_size.x;
	w_center_height = w_height / 2;
	w_center_width = w_width / 2;
}

void Space::Zoom(int amount)
{
	float new_zoom = amount > 0 ? zoom_factor - ZOOM_SPEED : zoom_factor + ZOOM_SPEED;
	zoom_factor = M::FClamp(new_zoom, 100, 10000);
}

void Space::UpdateTime()
{
	runtime = runtimeClock.getElapsedTime().asMilliseconds() / 1000.f;
	deltatime = deltaClock.getElapsedTime().asMilliseconds() / 1000.f;
	deltaClock.restart();
}

void Space::ProcessItems()
{
	for (Node* node : m_nodes)
	{
		node->Tick(deltatime);
		if (node->lifetime > 5) SplitNode(node, 500);
	}
	for (Link* link : m_links)
	{
		link->Tick(deltatime);
	}
}

void Space::HandleInput()
{
	float speed = 4;
	// Debugging - movement controls
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
	{
		camera_rotation.x += CAMERA_TURN_SPEED * deltatime;
	}
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
	{
		camera_rotation.x -= CAMERA_TURN_SPEED * deltatime;
	}
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up))
	{
		camera_rotation.y = M::FClamp(camera_rotation.y + (CAMERA_TURN_SPEED * deltatime), -1.5708f, 1.5708f); // Lock camera between -90 and 90 degrees
	}
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down))
	{
		camera_rotation.y = M::FClamp(camera_rotation.y - (CAMERA_TURN_SPEED * deltatime), -1.5708f, 1.5708f);
	}
}

// Render out nodes in the tree
void Space::Render()
{
	if (m_nodes.empty()) return;

	std::sort(m_nodes.begin(), m_nodes.end(), CompareNodesByDepth); // Sort nodes by depth before rendering

	for (Node* node : m_nodes) // Render all nodes
	{
		drawNode(*node);
	}
	for (Link* link : m_links) // Render all links
	{
		drawLink(*link);
	}
}

void Space::UpdateFrameTimer()
{
	double currentTime = runtime;
	deltaFrames++;
	if (currentTime - lastTime >= 1.0) { // If last prinf() was more than 1 sec ago
		float secondsPerFrame = (float)(1000.0 / double(deltaFrames));
		int framerate = (1 / secondsPerFrame) * 1000;
		printf("%f ms/frame (%d FPS)\n", secondsPerFrame, framerate);
		deltaFrames = 0;
		lastTime += 1.0;
	}
}

void Space::drawNode(Node& node)
{
	node.view_location = applyCameraRotation(node);

	float scale = depthToScaleFactor(node.view_location.z) * node.m_size / 100;
	float tex_center = t_node_size / 2;

	// ??? : should this just be one global sprite that is drawn many times per frame?
	sf::Sprite tmp_sprite;
	tmp_sprite.setTexture(t_node);
	tmp_sprite.setOrigin(sf::Vector2f(tex_center, tex_center)); // Set origin to center of texture
	tmp_sprite.setScale(sf::Vector2f(scale, scale)); // Scale based on size and distance
	sf::Vector2f screen_position = toScreenSpace(node.view_location);
	node.view_location = sf::Vector3f(screen_position.x, screen_position.y, node.view_location.z);
	tmp_sprite.setPosition(screen_position); //Convert pos to screen position

	if (node.m_color) tmp_sprite.setColor(*node.m_color);

	window->draw(tmp_sprite);
}

sf::Vector2f Space::toScreenSpace(const sf::Vector3f & position)
{
	float depth_offset = ((0.001f * position.z * DEPTH_OFFSET_FACTOR) + 1);
	depth_offset = M::FClamp(depth_offset, 0.f, 100000.f);

	float screen_x = w_center_width + ((position.x / zoom_factor) * w_center_height * depth_offset);
	float screen_y = w_center_height + ((position.y / zoom_factor) * -w_center_height * depth_offset);

	return sf::Vector2f(screen_x, screen_y);
}

void Space::drawLink(Link & link)
{
	if (!link.a || !link.b) return; // Don't draw if link is broken. TODO [handle broken link]

	float zoom_scale = 1000.f / zoom_factor;

	sf::Vector3f pos_a = link.a->view_location;
	sf::Vector3f pos_b = link.b->view_location;
	sf::Vector3f midpt = CalcMidpoint(pos_a, pos_b);

	float distance = M::FClamp(GetDistance(sf::Vector2f(pos_a.x, pos_a.y), sf::Vector2f(pos_b.x, pos_b.y)) - (70 * zoom_scale), 0, 100000);
	float angle = GetAngle(sf::Vector2f(pos_a.x, pos_a.y), sf::Vector2f(pos_b.x, pos_b.y));

	sf::RectangleShape line(sf::Vector2f(link.thickness, distance));
	line.setOrigin(sf::Vector2f(link.thickness / 2, distance / 2));
	line.setFillColor(sf::Color(255, 255, 255, 100));
	line.setRotation(angle);

	line.setPosition(sf::Vector2f(midpt.x, midpt.y));

	window->draw(line);
}

void Space::AddNode(Node * node)
{
	m_nodes.push_back(node);
	// Other adding effects here
}

void Space::AddChild(Node * parent, Node * child)
{
	parent->AddChild(child);
	m_nodes.push_back(child);
}

void Space::SplitNode(Node * original, float force)
{
	Node* newnode = new Node();
	Link* link = new Link(original, newnode);
	AddLink(link);
	
	original->lifetime = 0;

	newnode->location = original->location;
	float randx = rand() % 100;
	float randy = rand() % 100;
	float randz = rand() % 100;
	printf("%f\n", randx);
	newnode->location.x += randx;
	newnode->location.y += randy;
	newnode->location.z += randz;

	original->m_size /= 2;
	newnode->m_size = original->m_size;

	AddNode(newnode);
}

void Space::AddLink(Link * link)
{
	m_links.push_back(link);
}

float Space::depthToScaleFactor(const float & depth)
{
	float res_scale = w_height / 720.f;
	float zoom_scale = 1000.f / zoom_factor;
	float factor = (float)((0.001 * depth * DEPTH_SCALE_FACTOR) + 1);
	factor *= zoom_scale;
	factor *= res_scale;
	return factor;
}

sf::Vector3f Space::applyCameraRotation(const Node & node)
{
	return RotateAroundOrigin(node.location, camera_rotation);
}
