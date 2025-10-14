#include "../dependencies/proprietary/boilerplate.h"

/* ------------------------- *
   -    Simple Console     -
 * ------------------------- */

enum SEVERITY {
   TRACE = 0,
   WARNING ,
   FIXME   ,
   DEBUG   ,
   SUCCESS
};

enum LOGSOURCE {
   WNDW = 0, // Window
   RNDR,     // Renderer
   PHYS,     // Physics
   NETW      // Networking
};

const uint MAX_LOGQUEUE_ENTRIES = 64;

// Each log entry must be exactly 64 bytes
struct alignas(64) Log_Entry {
   uint8 source;
   uint8 severity;
   char  text[62];
};

struct LogQueue {
   Log_Entry entries[MAX_LOGQUEUE_ENTRIES];
   uint write_idx; // write index
};

struct GameConsole {
   LogQueue logs;

   void add_entry(char* text, uint8 severity = TRACE, uint8 source = WNDW)
   {
      uint idx = logs.write_idx;

      // create a new entry
      logs.entries[idx].source   = source;
      logs.entries[idx].severity = severity;
      memcpy(logs.entries[idx].text, text, 62);

      // increment + wrap around if needed
      logs.write_idx = (logs.write_idx + 1) % MAX_LOGQUEUE_ENTRIES;
   }
} *console;

// This draws an imgui window for the console
void draw_console(GameConsole* console)
{
   LogQueue logs = console->logs;

   // Source colors — modern, balanced, and consistent brightness
   const ImVec4 source_colors[4] = {
       ImVec4(0.45f, 0.65f, 1.00f, 1.0f), // WNDW : Azure Blue
       ImVec4(0.75f, 0.60f, 1.00f, 1.0f), // RNDR : Vivid Violet
       ImVec4(1.00f, 0.55f, 0.45f, 1.0f), // PHYS : Coral Rust
       ImVec4(1.00f, 0.80f, 0.55f, 1.0f)  // NETW : Signal Amber
   };

   // Severity levels — distinct, modern color language
   const ImVec4 severity_colors[5] = {
       ImVec4(0.75f, 0.55f, 1.00f, 1.0f), // TRACE   : Purple (Insight)
       ImVec4(1.00f, 0.90f, 0.45f, 1.0f), // WARNING : Yellow (Caution)
       ImVec4(1.00f, 0.55f, 0.20f, 1.0f), // FIXME   : Orange (Problem)
       ImVec4(0.45f, 0.70f, 1.00f, 1.0f), // DEBUG   : Blue (Info)
       ImVec4(0.45f, 1.00f, 0.55f, 1.0f)  // SUCCESS : Green (OK)
   };

   // Message text — softened tints of above for legibility on dark backgrounds
   const ImVec4 message_colors[5] = {
       ImVec4(0.85f, 0.70f, 1.00f, 1.0f), // TRACE   : Soft Lavender
       ImVec4(1.00f, 0.95f, 0.70f, 1.0f), // WARNING : Pale Yellow
       ImVec4(1.00f, 0.75f, 0.50f, 1.0f), // FIXME   : Soft Orange
       ImVec4(0.70f, 0.85f, 1.00f, 1.0f), // DEBUG   : Light Blue
       ImVec4(0.70f, 1.00f, 0.75f, 1.0f)  // SUCCESS : Soft Green
   };

   const char* severity_messages[5] = {
       "T", // trace
       "W", // warning
       "F", // fixme
       "D", // debug
       "S"  // success
   };

   const char* source_messages[4] = {
       "WNDW", // Window
       "RNDR", // Renderer
       "PHYS", // Physics
       "NETW"  // Networking
   };

   // imgui window settings
   ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse
      | ImGuiWindowFlags_NoResize
      | ImGuiWindowFlags_NoMove
      | ImGuiWindowFlags_NoScrollbar;

   // imgui window -> background color
   ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.1f, 0.5f)); // RGBA

   // create imgui window
   ImGui::Begin("GameConsole", (bool*)1, flags);
   ImGui::SetWindowPos(ImVec2(0, 0));
   ImGui::SetWindowSize(ImVec2(540, 1080));

   // loop to print newest->oldest, we do 2 loops because we're using a rolling buffer
   for (uint i = logs.write_idx; i < MAX_LOGQUEUE_ENTRIES; i++)
   {
      if (logs.entries[i].text[0] == 0) continue; // don't draw empty strings

      uint severity = logs.entries[i].severity;
      uint source = logs.entries[i].source;

      // Print the source, severity, and message with appropriate colors
      ImGui::TextColored(source_colors[source], "[%-4s]", source_messages[source]);
      ImGui::SameLine();
      ImGui::TextColored(severity_colors[severity], "[%s]", severity_messages[severity]);
      ImGui::SameLine();
      ImGui::TextColored(message_colors[severity], "[%s]", logs.entries[i].text);
   }

   for (uint i = 0; i < logs.write_idx; i++)
   {
      if (logs.entries[i].text[0] == 0) continue; // don't draw empty strings

      uint severity = logs.entries[i].severity;
      uint source = logs.entries[i].source;

      ImGui::TextColored(source_colors[source], "[%-4s]", source_messages[source]);
      ImGui::SameLine();
      ImGui::TextColored(severity_colors[severity], "[%s]", severity_messages[severity]);
      ImGui::SameLine();
      ImGui::TextColored(message_colors[severity], "[%s]", logs.entries[i].text);
   }

   ImGui::SetScrollHereY(1.0f); // Scroll to bottom

   ImGui::End();
   ImGui::PopStyleColor();
}