# EECS498-Game Engine Architecture Final Customized Feature by Jin Pan

Explore the latest in game engine customization with our unique feature set, crafted by Jin Pan for the EECS498 Game Engine Architecture course. This custom feature is built on the latest web build milestone, showcasing a seamless integration of different HTML templates with a focus on flexibility and user choice.

## HTML Template Options

To address the challenge of building various user interfaces, we designed multiple HTML templates, each with distinct styles and functionalities. The main goal was to create an adaptable system allowing users to switch between these templates effortlessly. Here’s how we did it:

- **Switch and Build Scripts:** We implemented scripts like `switch_and_build.sh`, `Makefile`, and `setup_and_build.sh` that facilitate the switching process between different HTML templates.
- **Core Templates:** Our system features three core templates:
  - **Minimal Shell:** A simple, clean design for users who prefer a minimalistic approach.
  - **Dark Shell:** A dark-themed template that provides a sleek and modern interface.
  - **Retro Shell:** A nostalgic, old-school design with a retro feel.
- **Showcasing Templates:** Each template showcases unique CSS styles and uses Emscripten scripts to manage content display and background UI design. This flexibility offers users a variety of options to choose from, catering to diverse design preferences.

## Web Interactive Editor

To enhance user interaction, we developed a web-based interactive editor, enabling users to design custom game levels and maps. This feature was created to make the game engine architecture more engaging and user-friendly. Here's what it includes:

- **Server and Client Side:** Using `server.js` and `interactive_shell.html`, we established a seamless communication channel between the server and client sides. This connection allows users to create and edit game levels in real-time.
- **Re-compilation Process:** One of the challenges we encountered was the compilation issue when switching from one HTML template to another. We're considering automating the re-compilation process to ensure smoother transitions.

This final customized feature represents a step forward in game engine architecture, offering developers a flexible and interactive environment to create engaging content. With multiple templates and an interactive editor, users can design their ideal game levels and switch between various UI styles with ease.
