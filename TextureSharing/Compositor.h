#pragma once

class Compositor {
public:
	Compositor();
	~Compositor();

	void Composite();

private:
	void ReadTextures();

};