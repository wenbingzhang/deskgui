#include <deskgui/app.h>

#include <atomic>
#include <catch2/catch_all.hpp>
#include <chrono>
#include <memory>
#include <thread>

namespace {

  inline deskgui::ViewSize toDips(const deskgui::ViewSize& size, float scale) {
    return {static_cast<std::size_t>(size.first / scale),
            static_cast<std::size_t>(size.second / scale)};
  }

  inline deskgui::ViewRect toDips(const deskgui::ViewRect& rect, float scale) {
    return {static_cast<std::size_t>(rect.L / scale), static_cast<std::size_t>(rect.T / scale),
            static_cast<std::size_t>(rect.R / scale), static_cast<std::size_t>(rect.B / scale)};
  }

}  // namespace

TEST_CASE("Window basic functionality") {
  deskgui::App app;
  auto window = app.createWindow("window");
  REQUIRE(window);  // Ensure window creation succeeded

  const auto scale = window->getMonitorScaleFactor();

  SECTION("Native window handle is valid") { CHECK(window->getNativeWindow() != nullptr); }

  SECTION("Set and get title") {
    constexpr auto expectedTitle = "Window tests";
    window->setTitle(expectedTitle);
    CHECK(window->getTitle() == expectedTitle);
  }

  SECTION("Set and get size") {
    constexpr deskgui::ViewSize expectedSize{600, 600};
    window->setSize(expectedSize);
    CHECK(toDips(window->getSize(), scale) == expectedSize);
  }

  SECTION("Set and get max size") {
    constexpr deskgui::ViewSize expectedSize{600, 600};
    window->setMaxSize(expectedSize);
    CHECK(toDips(window->getMaxSize(), scale) == expectedSize);
  }

  SECTION("Set and get min size") {
    constexpr deskgui::ViewSize expectedSize{600, 600};
    window->setMinSize(expectedSize);
    CHECK(toDips(window->getMinSize(), scale) == expectedSize);
  }

  SECTION("Resizable flag") {
    window->setResizable(true);
    CHECK(window->isResizable());

    window->setResizable(false);
    CHECK_FALSE(window->isResizable());
  }

  SECTION("Set and get window position") {
    constexpr deskgui::ViewRect expectedPos{200, 100, 500, 600};
    window->setPosition(expectedPos);

    const auto actual = toDips(window->getPosition(), scale);
    INFO("Expected: (" << expectedPos.L << "," << expectedPos.T << "," << expectedPos.R << ","
                       << expectedPos.B << ")");
    INFO("Actual: (" << actual.L << "," << actual.T << "," << actual.R << "," << actual.B << ")");
    CHECK(actual == expectedPos);
  }

  SECTION("Decorations flag") {
    window->setDecorations(true);
    CHECK(window->isDecorated());

    window->setDecorations(false);
    CHECK_FALSE(window->isDecorated());
  }
}
