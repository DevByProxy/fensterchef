#ifndef FENSTERCHEF_CORE_APPLICATION_HPP
#define FENSTERCHEF_CORE_APPLICATION_HPP

#include <memory>
#include <string>

namespace fensterchef {

// Forward declarations
class WindowManager;
class DisplayManager;
class TilingManager;
class InputHandler;
class X11Connection;
class Config;
class Renderer;

/**
 * @brief Main application class for Fensterchef window manager
 * 
 * This class represents the entire window manager application and coordinates
 * all subsystems. It replaces the scattered initialization code in main.cpp
 * and fensterchef.cpp with a clean, object-oriented design.
 */
class Application {
public:
    /**
     * @brief Construct the application
     * @param argc Command line argument count
     * @param argv Command line arguments
     */
    Application(int argc, char** argv);
    
    /**
     * @brief Destructor - cleans up all subsystems
     */
    ~Application();
    
    // Non-copyable, non-movable
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&) = delete;
    Application& operator=(Application&&) = delete;
    
    /**
     * @brief Initialize all subsystems
     * @return true on success, false on failure
     */
    bool initialize();
    
    /**
     * @brief Run the main event loop
     * @return Exit code (0 for success)
     */
    int run();
    
    /**
     * @brief Request application shutdown
     * @param exit_code The exit code to return
     */
    void quit(int exit_code = 0);
    
    /**
     * @brief Check if application is running
     * @return true if running, false if quitting
     */
    bool isRunning() const noexcept { return is_running_; }
    
    /**
     * @brief Get the window manager
     */
    WindowManager& getWindowManager() { return *window_manager_; }
    const WindowManager& getWindowManager() const { return *window_manager_; }
    
    /**
     * @brief Get the display manager
     */
    DisplayManager& getDisplayManager() { return *display_manager_; }
    const DisplayManager& getDisplayManager() const { return *display_manager_; }
    
    /**
     * @brief Get the tiling manager
     */
    TilingManager& getTilingManager() { return *tiling_manager_; }
    const TilingManager& getTilingManager() const { return *tiling_manager_; }
    
    /**
     * @brief Get the input handler
     */
    InputHandler& getInputHandler() { return *input_handler_; }
    const InputHandler& getInputHandler() const { return *input_handler_; }
    
    /**
     * @brief Get the X11 connection
     */
    X11Connection& getX11Connection() { return *x11_connection_; }
    const X11Connection& getX11Connection() const { return *x11_connection_; }
    
    /**
     * @brief Get the configuration
     */
    Config& getConfig() { return *config_; }
    const Config& getConfig() const { return *config_; }
    
    /**
     * @brief Get the renderer
     */
    Renderer& getRenderer() { return *renderer_; }
    const Renderer& getRenderer() const { return *renderer_; }
    
private:
    /**
     * @brief Parse command line arguments
     * @return true on success
     */
    bool parseArguments(int argc, char** argv);
    
    /**
     * @brief Initialize X11 connection
     * @return true on success
     */
    bool initializeX11();
    
    /**
     * @brief Take control as window manager
     * @return true on success
     */
    bool takeControl();
    
    /**
     * @brief Initialize signal handlers
     * @return true on success
     */
    bool initializeSignalHandlers();
    
    /**
     * @brief Load configuration
     * @return true on success
     */
    bool loadConfiguration();
    
    /**
     * @brief Query and manage existing windows
     */
    void queryExistingWindows();
    
    /**
     * @brief Run startup actions
     */
    void runStartupActions();
    
    // Subsystems (in initialization order)
    std::unique_ptr<Config> config_;
    std::unique_ptr<X11Connection> x11_connection_;
    std::unique_ptr<WindowManager> window_manager_;
    std::unique_ptr<DisplayManager> display_manager_;
    std::unique_ptr<TilingManager> tiling_manager_;
    std::unique_ptr<InputHandler> input_handler_;
    std::unique_ptr<Renderer> renderer_;
    
    // State
    bool is_running_;
    int exit_code_;
    std::string config_file_path_;
};

} // namespace fensterchef

#endif // FENSTERCHEF_CORE_APPLICATION_HPP
