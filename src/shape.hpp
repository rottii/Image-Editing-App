#pragma once
#include <SFML/System/Vector2.hpp>

//Computes points positions for a circle
struct CircleGenerator
{
	static float constexpr pi{ 3.1415927f };
	float const radius = 0.0f;
	//The number of points to use
	uint32_t const quality = 0;
	//The angle variation between two consecutive points
	float const da = 0.0f;

	//The generator's construction
	CircleGenerator(float radius_, uint32_t quality_)
		: radius{ radius_ }
		, quality{ quality_ }
		, da{ (2.0f * pi) / static_cast<float>(quality) }
	{}

	//Returns the ith point of the perimeter
	sf::Vector2f getPoint(uint32_t i) const
	{
		//Compute the angle associated with the requested point
		float const angle{ da * static_cast<float>(i) };
		return { radius * sf::Vector2f{cos(angle), sin(angle)} };
	}
};

//Computes points positions for a rounded rectangle
struct RoundedRectangleGenerator
{
	sf::Vector2f const size;
	sf::Vector2f const centers[4];
	uint32_t const arc_quality;
	CircleGenerator const generator;

	RoundedRectangleGenerator(sf::Vector2f size_, float radius, uint32_t quality)
		: size{ size_ }
		, centers{
			{size.x - radius, size.y - radius}, //BR
			{radius, size.y - radius}, //BL
			{radius, radius}, //TL
			{size.x - radius, radius}, //TR
		}
		, arc_quality{ quality / 4 }
		, generator{ radius, quality - 4 }
	{}
	
	//Returns the ith point of the perimeter
	sf::Vector2f getPoint(uint32_t i) const
	{
		uint32_t const corner_idx{ i / arc_quality };
		return centers[corner_idx] + generator.getPoint(i - corner_idx);
	}
};