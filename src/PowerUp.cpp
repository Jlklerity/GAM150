#include "PowerUp.h"
#include <string>
#include <iostream>
#include <math.h>

// circle mesh

struct PowerUp {
	int index;
	std::string texture;
	int yvalue;

	PowerUp(int i, const std::string& t, int y)
		: index(i), texture(t), yvalue(y) {
	}
};

PowerUp lightning(1, "lightning.png", 450);
PowerUp timeslow(2, "timeslow.png", 450);

void Activate_PowerUp(PowerUp p) {
	int index = p.index;
	std::string texture = p.texture;
	int y = p.yvalue;

	std::cout << index << texture << y << '\n'; // debug print

	if (index == 1) {
		// draw animated lightning on screen
		// logic for all enemy on screen to die
	}

	if (index == 2) {
		// pause delta time increment
	}
}

void Cooldown_PowerUp(PowerUp p) {
	bool ready = false;
	double cooldown = 0.0f; // use delta time to increment

	if (cooldown <= 30.0f) {
		// draw translucent circle over the power up button indicating cooldown timer
		ready = false;
	}

	if (cooldown >= 30.0f) {
		ready = true;
	}

	if (ready == true) // && onclick == true
	{
		Activate_PowerUp(p);
		cooldown = 0.0f;
	}
}

void Spawn_PowerUp(PowerUp p) {
	double cooldown = 0.0f; // use delta time to increment
	int index = 1; // change to math.random
	int xvalue = 100; // change to math.random(100,1500)

	if (cooldown >= 15.0f) {
		// increment yvalue till floor boundary
		// draw powerup at xvalue, yvalue
		// if player clicks the power up
		// activate powerup
		// if hit floor boundary, disappear
	}
}