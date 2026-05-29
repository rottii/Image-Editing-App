#include <SFML/Graphics.hpp>
#include <vector>
#include <optional>
#include "portable-file-dialogs.h"
#include "events.hpp"//useless now
#include "configuration.hpp"
#include "shape.hpp"
#include "formulas.h"


//Adds circle points inside a pre-allocated vertex array
void generateCircle(sf::VertexArray& vertex_array, sf::Vector2f position, float radius, uint32_t quality, sf::Color color)
{
	//Create the generator
	CircleGenerator const generator{ radius, quality };
	//Add the points to the vertex array
	for (uint32_t i{ 0 }; i < quality; ++i)
	{
		vertex_array[i].position = position + generator.getPoint(i);
		vertex_array[i].color = color;
	}
}

void generateRoundedRectangle(sf::VertexArray& vertex_array, sf::Vector2f position, sf::Vector2f size, float radius, uint32_t quality, sf::Color color)
{
	//Create the generator
	RoundedRectangleGenerator const generator{ size, radius, quality };
	//Add the points to the vertex array
	for (uint32_t i{ 0 }; i < quality; ++i)
	{
		vertex_array[i].position = position + generator.getPoint(i);
		vertex_array[i].color = color;
	}
}

void generateOutline(sf::VertexArray& vertex_array, sf::Vector2f position, sf::Vector2f size, float radius, float thickness, uint32_t quality, sf::Color innerColor, sf::Color outerColor)
{
	//Create the two generators, one for the outside and other for the inside
	RoundedRectangleGenerator const generator_out{ size, radius, quality };
	sf::Vector2f const in_offset{ thickness, thickness };
	sf::Vector2f const in_size = size - 2.0f * in_offset;
	RoundedRectangleGenerator const generator_in{ in_size, radius - thickness, quality };
	//Add the points to the vertex array
	for (uint32_t i{ 0 }; i < quality; ++i)
	{
		vertex_array[2 * i + 0].position = position + in_offset + generator_in.getPoint(i);
		vertex_array[2 * i + 0].color = innerColor;
		vertex_array[2 * i + 1].position = position + generator_out.getPoint(i);
		vertex_array[2 * i + 1].color = outerColor;
	}
	//Add the two first points again to close the loop
	vertex_array[2 * quality + 0].position = position + in_offset + generator_in.getPoint(0);
	vertex_array[2 * quality + 0].color = innerColor;
	vertex_array[2 * quality + 1].position = position + generator_out.getPoint(0);
	vertex_array[2 * quality + 1].color = outerColor;
}

struct ShapeData {
	sf::VertexArray vertices;
	sf::VertexArray shadow;
};

// Ýki sayý arasýnda yumuþak geçiþ yapar
float lerp(float current, float target, float speed) {
	return current + (target - current) * speed;
}

// Ýki renk arasýnda yumuþak geçiþ yapar
sf::Color lerpColor(sf::Color current, sf::Color target, float speed) {
	return sf::Color(
		static_cast<uint8_t>(lerp(current.r, target.r, speed)),
		static_cast<uint8_t>(lerp(current.g, target.g, speed)),
		static_cast<uint8_t>(lerp(current.b, target.b, speed)),
		static_cast<uint8_t>(lerp(current.a, target.a, speed))
	);
}

sf::Color applyAlpha(sf::Color c, uint8_t alpha) {
	// We multiply the existing alpha by our animation alpha ratio
	c.a = static_cast<uint8_t>((c.a * alpha) / 255);
	return c;
}

int main()
{
	sf::ContextSettings settings;
	settings.antiAliasingLevel = 4;
    sf::RenderWindow window(sf::VideoMode({ conf::window_size.x, conf::window_size.y }), "Image Editing App", sf::State::Fullscreen, settings);
    window.setFramerateLimit(conf::max_framerate);

	std::optional<sf::Sprite> canvasSprite;//We put optional to create it empty
	sf::RenderTexture photoCanvas;
	sf::Texture photoTexture;
	std::string uploadedFilePath = "";

	#pragma region Buttons
	sf::Font font;
	font.openFromFile("res/fonts/arial.ttf");

	bool isMenuOpen = false;
	float menuAnimProgress = 0.f; // 0.f = fully closed, 1.f = fully open
	float slideDistance = 40.f;   // How many pixels the buttons slide down

	uint32_t const quality = 60;
	sf::Color shadowCol = sf::Color(0, 0, 0, 100);

	//Menu Toggle button 
	sf::Text menuBtnText(font, "Menu", 24);
	menuBtnText.setFillColor(sf::Color::White);

	ShapeData menuBtn;
	menuBtn.vertices.setPrimitiveType(sf::PrimitiveType::TriangleFan);
	menuBtn.vertices.resize(quality);
	menuBtn.shadow.setPrimitiveType(sf::PrimitiveType::TriangleFan);
	menuBtn.shadow.resize(quality);

	sf::FloatRect menuBtnBounds({ 30.f, 30.f }, { 200.f, 60.f });
	sf::Color menuBtnColorNormal = sf::Color(40, 40, 40);
	sf::Color menuBtnColorHover = sf::Color(70, 70, 70);
	sf::Color menuBtnColorClick = sf::Color(20, 20, 20);

	generateRoundedRectangle(menuBtn.vertices, { menuBtnBounds.position.x, menuBtnBounds.position.y }, { menuBtnBounds.size.x, menuBtnBounds.size.y }, 10.f, quality, menuBtnColorNormal);
	generateRoundedRectangle(menuBtn.shadow, { menuBtnBounds.position.x + 3.f, menuBtnBounds.position.y + 5.f }, { menuBtnBounds.size.x, menuBtnBounds.size.y }, 10.f, quality, shadowCol);
	menuBtnText.setPosition({ menuBtnBounds.position.x + 35.f, menuBtnBounds.position.y + 15.f });

	//Open Image button
	sf::Text openBtnText(font, "Open Image", 24);
	openBtnText.setFillColor(sf::Color::White);

	ShapeData openBtn;
	openBtn.vertices.setPrimitiveType(sf::PrimitiveType::TriangleFan);
	openBtn.vertices.resize(quality);
	openBtn.shadow.setPrimitiveType(sf::PrimitiveType::TriangleFan);
	openBtn.shadow.resize(quality);

	sf::FloatRect openBtnBounds({ 30.f, 100.f }, { 200.f, 60.f });
	sf::Color openBtnColorNormal = sf::Color(0, 120, 215);
	sf::Color openBtnColorHover = sf::Color(50, 150, 255);
	sf::Color openBtnColorClick = sf::Color(0, 80, 160);

	generateRoundedRectangle(openBtn.vertices, { openBtnBounds.position.x, openBtnBounds.position.y }, { openBtnBounds.size.x, openBtnBounds.size.y }, 10.f, quality, openBtnColorNormal);
	generateRoundedRectangle(openBtn.shadow, { openBtnBounds.position.x + 3.f, openBtnBounds.position.y + 5.f}, { openBtnBounds.size.x, openBtnBounds.size.y }, 10.f, quality, shadowCol);

	openBtnText.setPosition({ openBtnBounds.position.x + 35.f, openBtnBounds.position.y + 15.f });

	//Warp Image button
	sf::Text warpBtnText(font, "Warp Image", 24);
	warpBtnText.setFillColor(sf::Color::White);

	ShapeData warpBtn;
	warpBtn.vertices.setPrimitiveType(sf::PrimitiveType::TriangleFan);
	warpBtn.vertices.resize(quality);
	warpBtn.shadow.setPrimitiveType(sf::PrimitiveType::TriangleFan);
	warpBtn.shadow.resize(quality);

	sf::FloatRect warpBtnBounds({ 30.f, 170.f }, { 200.f, 60.f });
	sf::Color warpBtnColorNormal = sf::Color(0, 120, 215);
	sf::Color warpBtnColorHover = sf::Color(50, 150, 255);
	sf::Color warpBtnColorClick = sf::Color(0, 80, 160);

	generateRoundedRectangle(warpBtn.vertices, { warpBtnBounds.position.x, warpBtnBounds.position.y }, { warpBtnBounds.size.x, warpBtnBounds.size.y }, 10.f, quality, warpBtnColorNormal);
	generateRoundedRectangle(warpBtn.shadow, { warpBtnBounds.position.x + 3.f, warpBtnBounds.position.y + 5.f}, { warpBtnBounds.size.x, warpBtnBounds.size.y }, 10.f, quality, shadowCol);

	warpBtnText.setPosition({ warpBtnBounds.position.x + 35.f, warpBtnBounds.position.y + 15.f });

	//Find Corners button
	sf::Text cornersBtnText(font, "Find Corners", 24);
	cornersBtnText.setFillColor(sf::Color::White);

	ShapeData cornersBtn;
	cornersBtn.vertices.setPrimitiveType(sf::PrimitiveType::TriangleFan);
	cornersBtn.vertices.resize(quality);
	cornersBtn.shadow.setPrimitiveType(sf::PrimitiveType::TriangleFan);
	cornersBtn.shadow.resize(quality);

	sf::FloatRect cornersBtnBounds({ 30.f, 240.f }, { 200.f, 60.f });
	sf::Color cornersBtnColorNormal = sf::Color(0, 120, 215);
	sf::Color cornersBtnColorHover = sf::Color(50, 150, 255);
	sf::Color cornersBtnColorClick = sf::Color(0, 80, 160);

	generateRoundedRectangle(cornersBtn.vertices, { cornersBtnBounds.position.x, cornersBtnBounds.position.y }, { cornersBtnBounds.size.x, cornersBtnBounds.size.y }, 10.f, quality, cornersBtnColorNormal);
	generateRoundedRectangle(cornersBtn.shadow, { cornersBtnBounds.position.x + 3.f, cornersBtnBounds.position.y + 5.f }, { cornersBtnBounds.size.x, cornersBtnBounds.size.y }, 10.f, quality, shadowCol);

	cornersBtnText.setPosition({ cornersBtnBounds.position.x + 35.f, cornersBtnBounds.position.y + 15.f });
	#pragma endregion
	#pragma region Button Animation prep

	sf::Clock deltaClock;

	//Open Image button
	float menuBtnCurrentShrink = 0.f;
	sf::Color menuBtnCurrentColor = menuBtnColorNormal;

	ShapeData animatedMenuBtn;
	animatedMenuBtn.vertices.setPrimitiveType(sf::PrimitiveType::TriangleFan);
	animatedMenuBtn.vertices.resize(quality);

	//Open Image button
	float openBtnCurrentShrink = 0.f; 
	sf::Color openBtnCurrentColor = openBtnColorNormal; 

	ShapeData animatedOpenBtn;
	animatedOpenBtn.vertices.setPrimitiveType(sf::PrimitiveType::TriangleFan);
	animatedOpenBtn.vertices.resize(quality);

	//Warp Image button
	float warpBtnCurrentShrink = 0.f;
	sf::Color warpBtnCurrentColor = warpBtnColorNormal;

	ShapeData animatedWarpBtn;
	animatedWarpBtn.vertices.setPrimitiveType(sf::PrimitiveType::TriangleFan);
	animatedWarpBtn.vertices.resize(quality);

	//Find Corners button
	float cornersBtnCurrentShrink = 0.f;
	sf::Color cornersBtnCurrentColor = cornersBtnColorNormal;

	ShapeData animatedCornersBtn;
	animatedCornersBtn.vertices.setPrimitiveType(sf::PrimitiveType::TriangleFan);
	animatedCornersBtn.vertices.resize(quality);
	#pragma endregion
	// --- KIRPMA / PENCERE ARACI DEÐÝÞKENLERÝ ---
	std::vector<sf::Vector2f> cropPoints = {
		{100.f, 100.f}, // Sol Üst
		{400.f, 100.f}, // Sað Üst
		{400.f, 400.f}, // Sað Alt
		{100.f, 400.f}  // Sol Alt
	};

	int draggedPointIndex = -1;
	float dynamicRadius = 4.f;

	// Görsel yuvarlak oluþturucu
	sf::CircleShape handleShape(dynamicRadius);
	handleShape.setFillColor(sf::Color::White);
	handleShape.setOutlineColor(sf::Color::Blue);

	// Create a camera specifically for the photo
	sf::View photoView = window.getDefaultView();
	// Panning variables
	bool isPanning = false;
	sf::Vector2i oldMousePos; // Remembers where the mouse was exactly 1 frame ago

	bool imgBoundaryLimit = true;

	//Normal fonksiyona sürekli parametre girmek gerekiyor diye bunu kullanýyoruz
	auto applyWarp = [&]()
		{
			// 1. Sort points to Top-Left, Top-Right, Bottom-Right, Bottom-Left
			double centerX = 0.0;
			double centerY = 0.0;

			for (int i = 0; i < 4; i++) {
				centerX += cropPoints[i].x;
				centerY += cropPoints[i].y;
			}
			centerX /= 4.0;
			centerY /= 4.0;

			std::sort(&cropPoints[0], &cropPoints[4],
				[centerX, centerY](const auto& a, const auto& b) {
					double angleA = std::atan2(a.y - centerY, a.x - centerX);
					double angleB = std::atan2(b.y - centerY, b.x - centerX);
					return angleA < angleB;
				});

			int width = photoTexture.getSize().x;
			int height = photoTexture.getSize().y;
			int outWidth = 0;
			int outHeight = 0;

			//Get image from GPU to RAM
			sf::Image imgData = photoCanvas.getTexture().copyToImage();
			const uint8_t* inputImageArray = imgData.getPixelsPtr();
			std::vector<uint8_t> outputImageArray;

			float inputPoints[8];
			for (int i = 0; i < 4; i++)
			{
				inputPoints[i * 2] = cropPoints[i].x;
				inputPoints[i * 2 + 1] = cropPoints[i].y;
			}

			warpImage(inputImageArray, outputImageArray, inputPoints, width, height, outWidth, outHeight);

			sf::Image newImage(sf::Vector2u(outWidth, outHeight), outputImageArray.data());

			photoTexture.loadFromImage(newImage);

			photoCanvas.resize(photoTexture.getSize());
			photoCanvas.clear(sf::Color::Transparent);

			sf::Sprite newRawSprite(photoTexture);
			photoCanvas.draw(newRawSprite);
			photoCanvas.display();

			canvasSprite.emplace(photoCanvas.getTexture());

			cropPoints[0] = { 0.f, 0.f };
			cropPoints[1] = { (float)outWidth, 0.f };
			cropPoints[2] = { (float)outWidth, (float)outHeight };
			cropPoints[3] = { 0.f, (float)outHeight };

			//Center the image on the window
			sf::Vector2f winSize(window.getSize().x, window.getSize().y);
			sf::Vector2f imgSize(photoTexture.getSize().x, photoTexture.getSize().y);

			photoView.setCenter({ imgSize.x / 2.f, imgSize.y / 2.f });

			float zoomFactor = std::max(imgSize.x / winSize.x, imgSize.y / winSize.y);
			photoView.setSize(winSize * zoomFactor);
		};

    while (window.isOpen())
    {
		// ==== EVENTS ====
		while (const std::optional event = window.pollEvent())
		{
			if (event->is<sf::Event::Closed>())
				window.close();

			// KEY PRESS EVENT 
			if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>())
			{
				if (keyPressed->code == sf::Keyboard::Key::Escape)
					window.close();

				if (keyPressed->code == sf::Keyboard::Key::Left && canvasSprite.has_value())
				{
					photoView.rotate(sf::degrees(90.f));
					draggedPointIndex = -1;
				}

				if (keyPressed->code == sf::Keyboard::Key::Right && canvasSprite.has_value())
				{
					photoView.rotate(sf::degrees(-90.f));
					draggedPointIndex = -1;
				}

				if (keyPressed->code == sf::Keyboard::Key::F)
				{
					//Center the image on the window
					sf::Vector2f winSize(window.getSize().x, window.getSize().y);
					sf::Vector2f imgSize(photoTexture.getSize().x, photoTexture.getSize().y);

					photoView.setCenter({ imgSize.x / 2.f, imgSize.y / 2.f });

					float zoomFactor = std::max(imgSize.x / winSize.x, imgSize.y / winSize.y);
					photoView.setSize(winSize * zoomFactor);
				}

				if (keyPressed->code == sf::Keyboard::Key::Space && canvasSprite.has_value())
				{
					applyWarp();
				}

				if (keyPressed->code == sf::Keyboard::Key::Enter && canvasSprite.has_value())
				{
					int width = photoTexture.getSize().x;
					int height = photoTexture.getSize().y;

					sf::Image imgData = photoCanvas.getTexture().copyToImage();
					const uint8_t* inputImageArray = imgData.getPixelsPtr();
					std::vector<uint8_t> outputImageArray;

					findFourCorners(inputImageArray, outputImageArray, width, height, cropPoints);

					sf::Image newImage(sf::Vector2u(width, height), outputImageArray.data());

					photoTexture.loadFromImage(imgData);

					photoCanvas.resize(photoTexture.getSize());
					photoCanvas.clear(sf::Color::Transparent);

					sf::Sprite newRawSprite(photoTexture);
					photoCanvas.draw(newRawSprite);
					photoCanvas.display();

					canvasSprite.emplace(photoCanvas.getTexture());

					//Center the image on the window
					sf::Vector2f winSize(window.getSize().x, window.getSize().y);
					sf::Vector2f imgSize(photoTexture.getSize().x, photoTexture.getSize().y);

					photoView.setCenter({ imgSize.x / 2.f, imgSize.y / 2.f });

					float zoomFactor = std::max(imgSize.x / winSize.x, imgSize.y / winSize.y);
					photoView.setSize(winSize* zoomFactor);

					//applyWarp();
				}

				if (keyPressed->code == sf::Keyboard::Key::S && keyPressed->control && canvasSprite.has_value())
				{

					auto destination = pfd::save_file("Save Image As", ".",
						{ "Image Files", "*.png *.jpg *.jpeg *.bmp" }).result();

					if (!destination.empty())
					{
						std::filesystem::path savePath = std::filesystem::u8path(destination);

						//Force an extension if the user didn't type one
						if (!savePath.has_extension())
						{
							savePath.replace_extension(".png"); // Default to PNG format
						}

						//Pull pixels from GPU and save
						sf::Image finalImage = photoCanvas.getTexture().copyToImage();

						if (finalImage.saveToFile(savePath))
						{
							printf("Saved successfully to: %s\n", savePath.string().c_str());
						}
						else
						{
							printf("SFML failed to save the image.\n");
						}
					}
				}
			}

			// MOUSE SCROLL EVENT 
			if (const auto* scroll = event->getIf<sf::Event::MouseWheelScrolled>())
			{
				if (scroll->wheel == sf::Mouse::Wheel::Vertical)
				{
					sf::Vector2i pixelPos = sf::Mouse::getPosition(window);
					sf::Vector2f beforeZoom = window.mapPixelToCoords(pixelPos, photoView);

					float zoomFactor = 1.0f;
					if (scroll->delta > 0) {
						zoomFactor = 0.9f; //Zoom in
					}
					else if (scroll->delta < 0) {
						zoomFactor = 1.1f; //Zoom out
					}
					photoView.zoom(zoomFactor);

					sf::Vector2f afterZoom = window.mapPixelToCoords(pixelPos, photoView);
					sf::Vector2f offset = beforeZoom - afterZoom;
					photoView.move(offset);
				}
			}

			// MOUSE PRESS EVENT 
			if (const auto* mouseClick = event->getIf<sf::Event::MouseButtonPressed>())
			{
				if (mouseClick->button == sf::Mouse::Button::Middle)
				{
					isPanning = true;
					oldMousePos = mouseClick->position;
				}

				if (mouseClick->button == sf::Mouse::Button::Left && canvasSprite.has_value())
				{
					sf::Vector2f worldPos = window.mapPixelToCoords(mouseClick->position, photoView);

					float viewScale = photoView.getSize().x / window.getDefaultView().getSize().x;
					float dynamicRadius = 4.f * viewScale;

					for (int i = 0; i < 4; ++i)
					{
						sf::FloatRect pointBounds({ cropPoints[i].x - dynamicRadius, cropPoints[i].y - dynamicRadius }, {dynamicRadius * 2, dynamicRadius * 2});

						if (pointBounds.contains(worldPos))
						{
							draggedPointIndex = i; 
							break;
						}
					}
				}
			}

			// MOUSE RELEASE EVENT 
			if (const auto* mouseRelease = event->getIf<sf::Event::MouseButtonReleased>())
			{
				if (mouseRelease->button == sf::Mouse::Button::Middle)
				{
					isPanning = false;
				}

				if (mouseRelease->button == sf::Mouse::Button::Left)
				{
					draggedPointIndex = -1; // To stop moving the points
					sf::Vector2f mousePos(static_cast<float>(mouseRelease->position.x), static_cast<float>(mouseRelease->position.y));

					bool clickHandled = false; // Flag to stop double-clicking overlapping buttons

					// 1. Check Menu Button (Highest Priority)
					if (canvasSprite.has_value() && menuBtnBounds.contains(mousePos))
					{
						isMenuOpen = !isMenuOpen;
						clickHandled = true;
					}

					// 2. Check Tool Buttons ONLY if the menu wasn't just toggled
					if (!clickHandled)
					{
						// Open button works if menu is open, OR if no image is loaded yet
						if ((!canvasSprite.has_value() || isMenuOpen) && openBtnBounds.contains(mousePos))
						{
							// Open File Dialog
							auto f = pfd::open_file("Select an Image", ".",
								{ "Image Files", "*.png *.jpg *.jpeg *.bmp", "All Files", "*" });

							if (!f.result().empty())
							{
								std::filesystem::path cleanPath = std::filesystem::u8path(f.result()[0]);
								if (photoTexture.loadFromFile(cleanPath))
								{
									photoCanvas.resize(photoTexture.getSize());
									sf::Sprite rawPhotoSprite(photoTexture);
									int width = photoTexture.getSize().x;
									int height = photoTexture.getSize().y;

									photoCanvas.clear(sf::Color::Transparent);
									photoCanvas.draw(rawPhotoSprite);
									photoCanvas.display();

									canvasSprite.emplace(photoCanvas.getTexture());

									cropPoints[0] = { 0.f, 0.f };
									cropPoints[1] = { (float)width, 0.f };
									cropPoints[2] = { (float)width, (float)height };
									cropPoints[3] = { 0.f, (float)height };

									sf::Vector2f winSize(window.getSize().x, window.getSize().y);
									sf::Vector2f imgSize(photoTexture.getSize().x, photoTexture.getSize().y);
									photoView.setCenter({ imgSize.x / 2.f, imgSize.y / 2.f });
									float zoomFactor = std::max(imgSize.x / winSize.x, imgSize.y / winSize.y);
									photoView.setSize(winSize * zoomFactor);

									isMenuOpen = false;
								}
							}
							clickHandled = true;
						}

						// Warp and Corners only function if image is loaded AND menu is open
						if (canvasSprite.has_value() && isMenuOpen)
						{
							if (!clickHandled && warpBtnBounds.contains(mousePos))
							{
								applyWarp();
								clickHandled = true;
							}

							if (!clickHandled && cornersBtnBounds.contains(mousePos))
							{
								int width = photoTexture.getSize().x;
								int height = photoTexture.getSize().y;

								sf::Image imgData = photoCanvas.getTexture().copyToImage();
								const uint8_t* inputImageArray = imgData.getPixelsPtr();
								std::vector<uint8_t> outputImageArray;

								findFourCorners(inputImageArray, outputImageArray, width, height, cropPoints);
								sf::Image newImage(sf::Vector2u(width, height), outputImageArray.data());
								photoTexture.loadFromImage(imgData);

								photoCanvas.resize(photoTexture.getSize());
								photoCanvas.clear(sf::Color::Transparent);
								sf::Sprite newRawSprite(photoTexture);
								photoCanvas.draw(newRawSprite);
								photoCanvas.display();

								canvasSprite.emplace(photoCanvas.getTexture());

								sf::Vector2f winSize(window.getSize().x, window.getSize().y);
								sf::Vector2f imgSize(photoTexture.getSize().x, photoTexture.getSize().y);
								photoView.setCenter({ imgSize.x / 2.f, imgSize.y / 2.f });
								float zoomFactor = std::max(imgSize.x / winSize.x, imgSize.y / winSize.y);
								photoView.setSize(winSize * zoomFactor);

								clickHandled = true;
							}
						}
					}
				}
			}

			// MOUSE MOVE EVENT 
			if (const auto* mouseMove = event->getIf<sf::Event::MouseMoved>())
			{
				if (isPanning)
				{
					sf::Vector2f oldPos = window.mapPixelToCoords(oldMousePos, photoView);
					sf::Vector2f newPos = window.mapPixelToCoords(mouseMove->position, photoView);

					sf::Vector2f delta = oldPos - newPos;
					photoView.move(delta);
					oldMousePos = mouseMove->position;
				}

				if (draggedPointIndex != -1)
				{
					sf::Vector2f newWorldPos = window.mapPixelToCoords(mouseMove->position, photoView);

					if (canvasSprite.has_value() && imgBoundaryLimit)
					{
						// Get the boundaries of the loaded image
						float maxX = static_cast<float>(photoTexture.getSize().x);
						float maxY = static_cast<float>(photoTexture.getSize().y);

						// Clamp the X and Y coordinates so they cannot go outside the texture
						newWorldPos.x = std::clamp(newWorldPos.x, 0.f, maxX);
						newWorldPos.y = std::clamp(newWorldPos.y, 0.f, maxY);
					}

					cropPoints[draggedPointIndex] = newWorldPos;
				}
			}
		}

		float dt = deltaClock.restart().asSeconds();
		if (dt > 0.1f) dt = 0.1f;

		// 3. Farenin durumuna göre "Hedefleri" deðiþtir
		sf::Vector2i mousePosI = sf::Mouse::getPosition(window);
		sf::Vector2f mousePos(static_cast<float>(mousePosI.x), static_cast<float>(mousePosI.y));

		float animSpeed = 15.f * dt;

		// --- 1. UPDATE MENU PROGRESS ---
				// If no image is loaded, force progress to 1 so the Open button is fully visible!
		float targetProgress = (isMenuOpen || !canvasSprite.has_value()) ? 1.f : 0.f;
		menuAnimProgress = lerp(menuAnimProgress, targetProgress, animSpeed);

		uint8_t menuAlpha = static_cast<uint8_t>(255.f * menuAnimProgress);
		float slideOffset = slideDistance * (1.f - menuAnimProgress); // Goes from 40 to 0

		// --- 2. MOVE HITBOXES DYNAMICALLY ---
		openBtnBounds.position.y = 100.f - slideOffset;
		warpBtnBounds.position.y = 170.f - slideOffset;
		cornersBtnBounds.position.y = 240.f - slideOffset;

		// --- 3. MENU TOGGLE BUTTON ANIMATION ---
		if (canvasSprite.has_value())
		{
			float targetShrinkMenu = 0.f;
			sf::Color targetColorMenu = menuBtnColorNormal;

			if (menuBtnBounds.contains(mousePos)) {
				if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
					targetColorMenu = menuBtnColorClick;
				}
				else {
					targetColorMenu = menuBtnColorHover;
					targetShrinkMenu = 1.f;
				}
			}
			menuBtnCurrentShrink = lerp(menuBtnCurrentShrink, targetShrinkMenu, animSpeed);
			menuBtnCurrentColor = lerpColor(menuBtnCurrentColor, targetColorMenu, animSpeed);
		}

		// --- 4. TOOL BUTTONS ANIMATION ---
		if (menuAnimProgress > 0.01f)
		{
			// Open Button Hover (always active if visible)
			float targetShrink = 0.f;
			sf::Color targetColor = openBtnColorNormal;
			if (openBtnBounds.contains(mousePos)) {
				targetColor = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left) ? openBtnColorClick : openBtnColorHover;
				targetShrink = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left) ? 0.f : 1.f;
			}
			openBtnCurrentShrink = lerp(openBtnCurrentShrink, targetShrink, animSpeed);
			openBtnCurrentColor = lerpColor(openBtnCurrentColor, targetColor, animSpeed);

			// Warp & Corners Hover (only active if an image is loaded)
			if (canvasSprite.has_value())
			{
				// Warp Hover
				float targetShrink1 = 0.f;
				sf::Color targetColor1 = warpBtnColorNormal;
				if (warpBtnBounds.contains(mousePos)) {
					targetColor1 = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left) ? warpBtnColorClick : warpBtnColorHover;
					targetShrink1 = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left) ? 0.f : 1.f;
				}
				warpBtnCurrentShrink = lerp(warpBtnCurrentShrink, targetShrink1, animSpeed);
				warpBtnCurrentColor = lerpColor(warpBtnCurrentColor, targetColor1, animSpeed);

				// Corners Hover
				float targetShrink2 = 0.f;
				sf::Color targetColor2 = cornersBtnColorNormal;
				if (cornersBtnBounds.contains(mousePos)) {
					targetColor2 = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left) ? cornersBtnColorClick : cornersBtnColorHover;
					targetShrink2 = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left) ? 0.f : 1.f;
				}
				cornersBtnCurrentShrink = lerp(cornersBtnCurrentShrink, targetShrink2, animSpeed);
				cornersBtnCurrentColor = lerpColor(cornersBtnCurrentColor, targetColor2, animSpeed);
			}
		}
		else
		{
			openBtnCurrentShrink = 0.f;
			warpBtnCurrentShrink = 0.f;
			cornersBtnCurrentShrink = 0.f;
		}

		// --- MENU TOGGLE BUTTON SHAPE (Only generate if image is loaded) ---
		if (canvasSprite.has_value()) {
			generateRoundedRectangle(
				animatedMenuBtn.vertices,
				{ menuBtnBounds.position.x + menuBtnCurrentShrink, menuBtnBounds.position.y + menuBtnCurrentShrink },
				{ menuBtnBounds.size.x - (2 * menuBtnCurrentShrink), menuBtnBounds.size.y - (2 * menuBtnCurrentShrink) },
				10.f, quality, menuBtnCurrentColor
			);
			menuBtnText.setPosition({ menuBtnBounds.position.x + 35.f, menuBtnBounds.position.y + 15.f });
		}

		// --- SUB-BUTTONS (Only generate if visible) ---
		if (menuAnimProgress > 0.01f)
		{
			// Apply current alpha to colors
			sf::Color currentOpenCol = applyAlpha(openBtnCurrentColor, menuAlpha);
			sf::Color currentWarpCol = applyAlpha(warpBtnCurrentColor, menuAlpha);
			sf::Color currentCornersCol = applyAlpha(cornersBtnCurrentColor, menuAlpha);

			// Apply current alpha to text
			openBtnText.setFillColor(applyAlpha(sf::Color::White, menuAlpha));
			warpBtnText.setFillColor(applyAlpha(sf::Color::White, menuAlpha));
			cornersBtnText.setFillColor(applyAlpha(sf::Color::White, menuAlpha));

			generateRoundedRectangle(
				animatedOpenBtn.vertices,
				{ openBtnBounds.position.x + openBtnCurrentShrink, openBtnBounds.position.y + openBtnCurrentShrink },
				{ openBtnBounds.size.x - (2 * openBtnCurrentShrink), openBtnBounds.size.y - (2 * openBtnCurrentShrink) },
				10.f, quality, currentOpenCol
			);

			// Re-sync text positions to animated bounds
			openBtnText.setPosition({ openBtnBounds.position.x + 35.f, openBtnBounds.position.y + 15.f });

			generateRoundedRectangle(
				animatedWarpBtn.vertices,
				{ warpBtnBounds.position.x + warpBtnCurrentShrink, warpBtnBounds.position.y + warpBtnCurrentShrink },
				{ warpBtnBounds.size.x - (2 * warpBtnCurrentShrink), warpBtnBounds.size.y - (2 * warpBtnCurrentShrink) },
				10.f, quality, currentWarpCol
			);
			warpBtnText.setPosition({ warpBtnBounds.position.x + 35.f, warpBtnBounds.position.y + 15.f });

			generateRoundedRectangle(
				animatedCornersBtn.vertices,
				{ cornersBtnBounds.position.x + cornersBtnCurrentShrink, cornersBtnBounds.position.y + cornersBtnCurrentShrink },
				{ cornersBtnBounds.size.x - (2 * cornersBtnCurrentShrink), cornersBtnBounds.size.y - (2 * cornersBtnCurrentShrink) },
				10.f, quality, currentCornersCol
			);
			cornersBtnText.setPosition({ cornersBtnBounds.position.x + 35.f, cornersBtnBounds.position.y + 15.f });

			// Animate Shadow Alpha
			sf::Color fadeShadow = applyAlpha(shadowCol, menuAlpha);
			for (size_t i = 0; i < openBtn.shadow.getVertexCount(); i++) openBtn.shadow[i].color = fadeShadow;
			for (size_t i = 0; i < warpBtn.shadow.getVertexCount(); i++) warpBtn.shadow[i].color = fadeShadow;
			for (size_t i = 0; i < cornersBtn.shadow.getVertexCount(); i++) cornersBtn.shadow[i].color = fadeShadow;
		}

		// === RENDER LOOP ===
		window.clear(sf::Color(40, 40, 40));

		// --- LAYER 1: THE PHOTO (Zoomable) ---
		// 1. Tell the window to look through the Photo Camera
		window.setView(photoView);

		// A. Draw the Photo Layer
		if (canvasSprite.has_value())
		{
			window.draw(*canvasSprite);//Draws the image

			sf::VertexArray lines(sf::PrimitiveType::LineStrip, 5);
			for (int i = 0; i < 4; ++i) {
				lines[i].position = cropPoints[i];
				lines[i].color = sf::Color::Cyan;
			}
			lines[4].position = cropPoints[0]; // Döngüyü kapat (Son noktayý ilkine baðla)
			lines[4].color = sf::Color::Cyan;

			window.draw(lines);

			float viewScale = photoView.getSize().x / window.getDefaultView().getSize().x;
			float dynamicRadius = 4.f * viewScale; 

			handleShape.setRadius(dynamicRadius);
			handleShape.setOrigin({ dynamicRadius, dynamicRadius });
			handleShape.setOutlineThickness(2.f * viewScale);

			for (int i = 0; i < 4; ++i) {
				handleShape.setPosition(cropPoints[i]);

				if (i == draggedPointIndex) {
					handleShape.setFillColor(sf::Color::Cyan);
				}
				else {
					handleShape.setFillColor(sf::Color::White);
				}

				window.draw(handleShape);
			}
		}

		//DRAW THE UI
		window.setView(window.getDefaultView());

		if (canvasSprite.has_value())
		{
			window.draw(menuBtn.shadow);
			window.draw(animatedMenuBtn.vertices);
			window.draw(menuBtnText);
		}

		// Only draw the sub-buttons if the animation is visible
		if (menuAnimProgress > 0.1f)
		{
			// We use a Transform to slide the static shadows down matching our offset
			sf::Transform shadowSlide;
			shadowSlide.translate({ 0.f, -slideOffset });

			window.draw(openBtn.shadow, shadowSlide);
			window.draw(animatedOpenBtn.vertices);
			window.draw(openBtnText);

			if (canvasSprite.has_value())
			{
				window.draw(warpBtn.shadow, shadowSlide);
				window.draw(animatedWarpBtn.vertices);
				window.draw(warpBtnText);

				window.draw(cornersBtn.shadow, shadowSlide);
				window.draw(animatedCornersBtn.vertices);
				window.draw(cornersBtnText);
			}
		}

		window.display();

		/*std::vector<ShapeData> shapes;
		uint32_t const quality = 60;

		for (int i = 0; i < 4; ++i) {
			ShapeData s;
			s.vertices.setPrimitiveType(sf::PrimitiveType::TriangleFan);
			s.vertices.resize(quality);

			s.shadow.setPrimitiveType(sf::PrimitiveType::TriangleFan);
			s.shadow.resize(quality);

			sf::Vector2f pos = { 30.f + (i * 470.f), 400.f };
			sf::Color col = sf::Color(170, 170, 170);

			sf::Vector2f shadowPos = { 30.f + (i * 470.f), 405.f };
			sf::Color shadowCol = sf::Color(0, 0, 0, 100);

			generateRoundedRectangle(s.vertices, pos, { 440.f, 600.f }, 15.f, quality, col);
			generateRoundedRectangle(s.shadow, shadowPos, { 440.f, 600.f }, 15.f, quality, shadowCol);
			shapes.push_back(s);
		}*/
		//// 2. Open the File Dialog!
		//// Parameters: Title, Default Path, Filters
		//auto f = pfd::open_file("Select an Image", ".",
		//	{ "Image Files (.png, .jpg, .jpeg, .bmp)", "*.png *.jpg *.jpeg *.bmp",
		//	  "All Files", "*" });
		//// 3. Check if the user selected a file or clicked "Cancel"
		//if (!f.result().empty())
		//{
		//	uploadedFilePath = f.result()[0]; // Grab the first file they selected
		//	std::cout << "User selected: " << uploadedFilePath << "\n";
		//}
		//else
		//	std::cout << "User canceled the file selection.\n";
		//if (!uploadedFilePath.empty())
		//{
		//	std::filesystem::path cleanPath = std::filesystem::u8path(uploadedFilePath);
		//	if (photoTexture.loadFromFile(cleanPath))
		//	{
		//		photoCanvas.resize(photoTexture.getSize());
		//		sf::Sprite rawPhotoSprite(photoTexture);
		//		photoCanvas.clear(sf::Color::Transparent);
		//		photoCanvas.draw(rawPhotoSprite);
		//		photoCanvas.display();
		//	
		//}
		// 2. Create the VertexArrays
		// TriangleFan is perfect for solid, convex shapes
		//sf::VertexArray rectVA(sf::PrimitiveType::TriangleFan, quality);
		//sf::VertexArray shadowVA(sf::PrimitiveType::TriangleFan, quality);
		//
		// TriangleStrip needs 2 points per quality step, plus 2 at the end to close the loop
		//sf::VertexArray outlineVA(sf::PrimitiveType::TriangleStrip, (2 * quality) + 2);
		//
		//sf::Vector2f position = { 300.f, 100.f };
		//sf::Vector2f shadowOffset = { 0.f, 5.f }; 
		//
		// 3. Generate the geometry
		//generateRoundedRectangle(rectVA, { 500.f, 500.f }, { 500.f, 200.f }, 20.f, quality, sf::Color(170, 170, 170));
		//generateRoundedRectangle(shadowVA, { 500.f, 507.f }, { 500.f, 200.f }, 20.f, quality, sf::Color(0, 0, 0, 100));
		//generateOutline(outlineVA, { 500.f, 500.f }, { 500.f, 500.f }, 20.f, 10.f, quality, sf::Color::White, sf::Color::Black);
		//
		// 4. Color the vertices so they aren't invisible (default is black/transparent)
		//for (std::size_t i = 0; i < outlineVA.getVertexCount(); ++i)
		//    outlineVA[i].color = sf::Color::Red;
		//window.clear(sf::Color(80, 80, 80));
		//sf::Sprite canvasSprite(photoCanvas.getTexture());
		//window.draw(canvasSprite);
		//for (const auto& s : shapes) {
		//	window.draw(s.shadow);
		//	window.draw(s.vertices);
		//}
        // 5. Draw the arrays
		//window.draw(outlineVA);
		//window.draw(shadowVA);
        //window.draw(rectVA);
        //window.display();
    }
}
