#include "application.hpp"
#include <print>

int main() {
  std::println("Hello Dulkan!");
  Application app;
  if (app.initialize()) {
    app.run();
  }

  app.close();
}
