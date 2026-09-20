#include "application.hpp"
#include <print>

int main() {
  Application app;
  if (app.initialize()) {
    app.run();
  }

  app.close();
  std::println("Hello world!");
}
