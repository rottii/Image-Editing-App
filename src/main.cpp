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

	//Open Image button
	sf::Text buttonText(font, "Open Image", 24);
	buttonText.setFillColor(sf::Color::White);

	uint32_t const quality = 60;
	ShapeData button;
	button.vertices.setPrimitiveType(sf::PrimitiveType::TriangleFan);
	button.vertices.resize(quality);
	button.shadow.setPrimitiveType(sf::PrimitiveType::TriangleFan);
	button.shadow.resize(quality);

	sf::FloatRect btnBounds({ 30.f, 30.f }, { 200.f, 60.f });
	sf::Color colorNormal = sf::Color(0, 120, 215);
	sf::Color colorHover = sf::Color(50, 150, 255);
	sf::Color colorClick = sf::Color(0, 80, 160);
	sf::Color shadowCol = sf::Color(0, 0, 0, 100);

	generateRoundedRectangle(button.vertices, { btnBounds.position.x, btnBounds.position.y }, { btnBounds.size.x, btnBounds.size.y }, 10.f, quality, colorNormal);
	generateRoundedRectangle(button.shadow, { btnBounds.position.x + 3.f, btnBounds.position.y + 5.f}, { btnBounds.size.x, btnBounds.size.y }, 10.f, quality, shadowCol);

	buttonText.setPosition({ btnBounds.position.x + 35.f, btnBounds.position.y + 15.f });

	//Warp Image button
	sf::Text buttonText1(font, "Warp Image", 24);
	buttonText1.setFillColor(sf::Color::White);

	ShapeData button1;
	button1.vertices.setPrimitiveType(sf::PrimitiveType::TriangleFan);
	button1.vertices.resize(quality);
	button1.shadow.setPrimitiveType(sf::PrimitiveType::TriangleFan);
	button1.shadow.resize(quality);

	sf::FloatRect btnBounds1({ 30.f, 30.f }, { 200.f, 60.f });
	sf::Color colorNormal1 = sf::Color(0, 120, 215);
	sf::Color colorHover1 = sf::Color(50, 150, 255);
	sf::Color colorClick1 = sf::Color(0, 80, 160);
	sf::Color shadowCol1 = sf::Color(0, 0, 0, 100);

	generateRoundedRectangle(button1.vertices, { btnBounds1.position.x, btnBounds1.position.y }, { btnBounds1.size.x, btnBounds1.size.y }, 10.f, quality, colorNormal1);
	generateRoundedRectangle(button1.shadow, { btnBounds1.position.x + 3.f, btnBounds1.position.y + 5.f}, { btnBounds1.size.x, btnBounds1.size.y }, 10.f, quality, shadowCol1);

	buttonText1.setPosition({ btnBounds1.position.x + 35.f, btnBounds1.position.y + 15.f });
	#pragma endregion
	#pragma region Button Animation prep
	//Open Image button
	sf::Clock deltaClock; // Geçen zamaný ölçecek
	float currentShrink = 0.f; // Butonun o anki "çökme" miktarý
	sf::Color currentColor = colorNormal; // Butonun o anki rengi

	ShapeData animatedBtn;
	animatedBtn.vertices.setPrimitiveType(sf::PrimitiveType::TriangleFan);
	animatedBtn.vertices.resize(quality);

	//Warp Image button
	float currentShrink1 = 0.f;
	sf::Color currentColor1 = colorNormal1;

	ShapeData animatedBtn1;
	animatedBtn1.vertices.setPrimitiveType(sf::PrimitiveType::TriangleFan);
	animatedBtn1.vertices.resize(quality);
	#pragma endregion
	// --- KIRPMA / PENCERE ARACI DEÐÝÞKENLERÝ ---
	std::vector<sf::Vector2f> cropPoints = {
		{100.f, 100.f}, // Sol Üst
		{400.f, 100.f}, // Sað Üst
		{400.f, 400.f}, // Sað Alt
		{100.f, 400.f}  // Sol Alt
	};

	int draggedPointIndex = -1; // Þu an hangi nokta sürükleniyor? (-1: Hiçbiri)
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

	auto applyWarp = [&]()
		{
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
			if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>())
			{
				if (keyPressed->code == sf::Keyboard::Key::Escape)
					window.close();

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
			}

			// MOUSE SCROLL EVENT (ZOOM)
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

			if (const auto* mouseRelease = event->getIf<sf::Event::MouseButtonReleased>())
			{

				if (mouseRelease->button == sf::Mouse::Button::Middle)
				{
					isPanning = false;
				}

				if (mouseRelease->button == sf::Mouse::Button::Left)
				{
					draggedPointIndex = -1;//To stop moving the points
					sf::Vector2f mousePos(static_cast<float>(mouseRelease->position.x), static_cast<float>(mouseRelease->position.y));

					if (btnBounds.contains(mousePos) && canvasSprite.has_value())
					{
						applyWarp();
					}

					if (btnBounds.contains(mousePos) && !canvasSprite.has_value())
					{
						// Open File Dialog
						auto f = pfd::open_file("Select an Image", ".",
							{ "Image Files", "*.png *.jpg *.jpeg *.bmp", "All Files", "*" });

						if (!f.result().empty())
						{
							std::filesystem::path cleanPath = std::filesystem::u8path(f.result()[0]);
							if (photoTexture.loadFromFile(cleanPath))
							{
								// Bake the photo onto the invisible canvas
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

								//Center the image on the window
								sf::Vector2f winSize(window.getSize().x, window.getSize().y);
								sf::Vector2f imgSize(photoTexture.getSize().x, photoTexture.getSize().y);

								photoView.setCenter({ imgSize.x / 2.f, imgSize.y / 2.f });

								float zoomFactor = std::max(imgSize.x / winSize.x, imgSize.y / winSize.y);
								photoView.setSize(winSize* zoomFactor);

								/*
								//To center the image on the  window
								sf::Vector2u imageSize = photoCanvas.getTexture().getSize();
								float imageCenterX = static_cast<float>(imageSize.x) / 2.f;
								float imageCenterY = static_cast<float>(imageSize.y) / 2.f;

								photoView = window.getDefaultView();

								sf::Vector2f currentCamFocus = photoView.getCenter();
								float currentCamFocusX = currentCamFocus.x;
								float currentCamFocusY = currentCamFocus.y;

								float zoomFactor = std::max(imageCenterX / currentCamFocusX, imageCenterY / currentCamFocusY);
								photoView.zoom(zoomFactor);

								sf::Vector2f offset = { imageCenterX - currentCamFocusX, imageCenterY - currentCamFocusY };
								photoView.move(offset);*/
							}
						}
					}
				}
			}

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

		//Open Image button
		float targetShrink = 0.f;
		sf::Color targetColor = colorNormal;
		sf::Vector2f targetTextOffset = { 0.f, 0.f };

		if (btnBounds.contains(mousePos)) {
			if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
				targetColor = colorClick;
				//targetShrink = 3.f; // Týklanýnca 3 piksel küçülsün
				//targetTextOffset = { 1.5f, 1.5f };
			}
			else {
				targetColor = colorHover;
				targetShrink = 1.f; 
			}
		}
		currentShrink = lerp(currentShrink, targetShrink, animSpeed);
		currentColor = lerpColor(currentColor, targetColor, animSpeed);

		//Warp Image button
		float targetShrink1 = 0.f;
		sf::Color targetColor1 = colorNormal1;

		if (btnBounds1.contains(mousePos)) {
			if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
				targetColor1 = colorClick1;
			}
			else {
				targetColor1 = colorHover1;
				targetShrink1 = 1.f;
			}
		}
		currentShrink1 = lerp(currentShrink1, targetShrink1, animSpeed);
		currentColor1 = lerpColor(currentColor1, targetColor1, animSpeed);


		// Yazýnýn konumu için de küçük bir animasyon (Ýsteðe baðlý)
		float textX = lerp(buttonText.getPosition().x, btnBounds.position.x + 35.f + targetTextOffset.x, animSpeed);
		float textY = lerp(buttonText.getPosition().y, btnBounds.position.y + 15.f + targetTextOffset.y, animSpeed);
		buttonText.setPosition({ textX, textY });

		// 5. ÞEKÝLLERÝ YENÝDEN OLUÞTUR (Animasyonlu deðerlerle)
		generateRoundedRectangle(
			animatedBtn.vertices,
			{ btnBounds.position.x + currentShrink, btnBounds.position.y + currentShrink },
			{ btnBounds.size.x - (2 * currentShrink), btnBounds.size.y - (2 * currentShrink) },
			10.f, quality, currentColor
		);

		generateRoundedRectangle(
			animatedBtn1.vertices,
			{ btnBounds1.position.x + currentShrink1, btnBounds1.position.y + currentShrink1 },
			{ btnBounds1.size.x - (2 * currentShrink1), btnBounds1.size.y - (2 * currentShrink1) },
			10.f, quality, currentColor1
		);

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

		// --- LAYER 2: THE UI (Fixed) ---
		// 2. IMPORTANT: Reset the camera back to normal before drawing the UI!
		window.setView(window.getDefaultView());

		if (!canvasSprite.has_value())
		{
			window.draw(button.shadow);
			window.draw(animatedBtn.vertices);
			window.draw(buttonText);
		}
		else
		{
			window.draw(button1.shadow);
			window.draw(animatedBtn1.vertices);
			window.draw(buttonText1);
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
