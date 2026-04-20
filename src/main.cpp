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

	sf::Font font;
	font.openFromFile("res/fonts/arial.ttf");
	sf::Text buttonText(font, "Open Image", 24);
	buttonText.setFillColor(sf::Color::White);

	uint32_t const quality = 60;
	ShapeData btnOpenImg;
	btnOpenImg.vertices.setPrimitiveType(sf::PrimitiveType::TriangleFan);
	btnOpenImg.vertices.resize(quality);
	btnOpenImg.shadow.setPrimitiveType(sf::PrimitiveType::TriangleFan);
	btnOpenImg.shadow.resize(quality);

	sf::FloatRect btnBounds({ 30.f, 30.f }, { 200.f, 60.f });
	sf::Color colorNormal = sf::Color(0, 120, 215);
	sf::Color colorHover = sf::Color(50, 150, 255);
	sf::Color colorClick = sf::Color(0, 80, 160);
	sf::Color shadowCol = sf::Color(0, 0, 0, 100);

	generateRoundedRectangle(btnOpenImg.vertices, { btnBounds.position.x, btnBounds.position.y }, { btnBounds.size.x, btnBounds.size.y }, 10.f, quality, colorNormal);
	generateRoundedRectangle(btnOpenImg.shadow, { btnBounds.position.x + 3.f, btnBounds.position.y + 5.f}, { btnBounds.size.x, btnBounds.size.y }, 10.f, quality, shadowCol);

	buttonText.setPosition({ btnBounds.position.x + 35.f, btnBounds.position.y + 15.f });

	// --- ANÝMASYON HAZIRLIÐI ---
	sf::Clock deltaClock; // Geçen zamaný ölçecek
	float currentShrink = 0.f; // Butonun o anki "çökme" miktarý
	sf::Color currentColor = colorNormal; // Butonun o anki rengi

	ShapeData animatedBtn;
	animatedBtn.vertices.setPrimitiveType(sf::PrimitiveType::TriangleFan);
	animatedBtn.vertices.resize(quality);

	// --- KIRPMA / PENCERE ARACI DEÐÝÞKENLERÝ ---
	// 4 adet köþe noktasý (Baþlangýçta ekranda rastgele bir kare oluþtursun)
	std::vector<sf::Vector2f> cropPoints = {
		{100.f, 100.f}, // Sol Üst
		{400.f, 100.f}, // Sað Üst
		{400.f, 400.f}, // Sað Alt
		{100.f, 400.f}  // Sol Alt
	};

	int draggedPointIndex = -1; // Þu an hangi nokta sürükleniyor? (-1: Hiçbiri)
	float handleRadius = 8.f;   // Tutamaklarýn (yuvarlaklarýn) büyüklüðü

	// Görsel yuvarlak oluþturucu
	sf::CircleShape handleShape(handleRadius);
	handleShape.setFillColor(sf::Color::White);
	handleShape.setOutlineThickness(2.f);
	handleShape.setOutlineColor(sf::Color::Blue);
	handleShape.setOrigin({ handleRadius, handleRadius }); // Merkezinden tutulabilmesi için

	// Create a camera specifically for the photo
	sf::View photoView = window.getDefaultView();
	// Panning variables
	bool isPanning = false;
	sf::Vector2i oldMousePos; // Remembers where the mouse was exactly 1 frame ago
    while (window.isOpen())
    {
		// EVENTS
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
					// Reset the photo camera to match the default window view
					photoView = window.getDefaultView();
				}

				if (keyPressed->code == sf::Keyboard::Key::Space)
				{
					//
				}
			}

			// MOUSE SCROLL EVENT (ZOOM)
			if (const auto* scroll = event->getIf<sf::Event::MouseWheelScrolled>())
			{
				// Make sure we are scrolling the vertical wheel
				if (scroll->wheel == sf::Mouse::Wheel::Vertical)
				{
					sf::Vector2i pixelPos = sf::Mouse::getPosition(window);
					sf::Vector2f beforeZoom = window.mapPixelToCoords(pixelPos, photoView);

					float zoomFactor = 1.0f;
					// scroll->delta is usually 1 (up) or -1 (down)
					if (scroll->delta > 0) {
						zoomFactor = 0.9f; // Zoom IN (Make the camera see 90% of what it used to)
					}
					else if (scroll->delta < 0) {
						zoomFactor = 1.1f; // Zoom OUT (Make the camera see 110% of what it used to)
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

					for (int i = 0; i < 4; ++i)
					{
						// Yuvarlaðýn sýnýrlarý içinde mi diye kontrol ediyoruz (Basit mesafe hesabý veya sf::FloatRect)
						sf::FloatRect pointBounds({ cropPoints[i].x - handleRadius, cropPoints[i].y - handleRadius }, {handleRadius * 2, handleRadius * 2});

						if (pointBounds.contains(worldPos))
						{
							draggedPointIndex = i; // Noktayý yakaladýk!
							break; // Döngüden çýk, birden fazla noktayý ayný anda tutmayalým
						}
					}
				}
			}

			if (const auto* mouseRelease = event->getIf<sf::Event::MouseButtonReleased>())
			{
				//For the panning
				if (mouseRelease->button == sf::Mouse::Button::Middle)
				{
					isPanning = false;
				}

				if (mouseRelease->button == sf::Mouse::Button::Left)
				{
					draggedPointIndex = -1;//To stop moving the points
					sf::Vector2f mousePos(static_cast<float>(mouseRelease->position.x), static_cast<float>(mouseRelease->position.y));

					//Check if mouse clicked inside the button bounds
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

								photoCanvas.clear(sf::Color::Transparent);
								photoCanvas.draw(rawPhotoSprite);
								photoCanvas.display();

								canvasSprite.emplace(photoCanvas.getTexture());
							}
						}
					}
				}
			}

			if (const auto* mouseMove = event->getIf<sf::Event::MouseMoved>())
			{
				if (isPanning)
				{
					// A. Convert the old and new mouse positions into the Camera's world coordinates
					sf::Vector2f oldPos = window.mapPixelToCoords(oldMousePos, photoView);
					sf::Vector2f newPos = window.mapPixelToCoords(mouseMove->position, photoView);

					// B. Calculate how much the mouse moved in the world
					// We do (old - new) instead of (new - old) because moving the mouse RIGHT 
					// means you are dragging the picture RIGHT, which means the CAMERA must move LEFT.
					sf::Vector2f delta = oldPos - newPos;

					// C. Move the camera
					photoView.move(delta);

					// D. Update the old mouse position for the next frame!
					oldMousePos = mouseMove->position;
				}

				if (draggedPointIndex != -1)
				{
					sf::Vector2f newWorldPos = window.mapPixelToCoords(mouseMove->position, photoView);
					cropPoints[draggedPointIndex] = newWorldPos;
				}
			}
		}

		// 1. Geçen zamaný al (Delta Time - dt)
		float dt = deltaClock.restart().asSeconds();

		if (dt > 0.1f) dt = 0.1f;

		// 2. Hedeflerimizi baþtan "Normal" olarak belirleyelim
		float targetShrink = 0.f;
		sf::Color targetColor = colorNormal;
		sf::Vector2f targetTextOffset = { 0.f, 0.f };

		// 3. Farenin durumuna göre "Hedefleri" deðiþtir
		sf::Vector2i mousePosI = sf::Mouse::getPosition(window);
		sf::Vector2f mousePos(static_cast<float>(mousePosI.x), static_cast<float>(mousePosI.y));

		if (btnBounds.contains(mousePos)) {
			if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
				targetColor = colorClick;
				//targetShrink = 3.f; // Týklanýnca 3 piksel küçülsün
				//targetTextOffset = { 1.5f, 1.5f };
			}
			else {
				targetColor = colorHover; // Üzerindeyken parlak mavi
				targetShrink = 1.f; // Hover olunca çok hafif küçülsün (tatlý bir hissiyat verir)
			}
		}

		// 4. ANÝMASYONU UYGULA (Mevcut deðerleri hedeflere doðru kaydýr)
		float animSpeed = 15.f * dt; // Hýz (Sayayý artýrýrsan animasyon hýzlanýr)

		currentShrink = lerp(currentShrink, targetShrink, animSpeed);
		currentColor = lerpColor(currentColor, targetColor, animSpeed);

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

		// === RENDER LOOP ===
		window.clear(sf::Color(40, 40, 40));

		// --- LAYER 1: THE PHOTO (Zoomable) ---
		// 1. Tell the window to look through the Photo Camera
		window.setView(photoView);

		// A. Draw the Photo Layer (Only if an image was actually loaded)
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

			for (int i = 0; i < 4; ++i) {
				handleShape.setPosition(cropPoints[i]);

				// Eðer bu nokta þu an sürükleniyorsa, rengini deðiþtirip vurgulayalým!
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
			window.draw(btnOpenImg.shadow);
			window.draw(animatedBtn.vertices);
			window.draw(buttonText);
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
