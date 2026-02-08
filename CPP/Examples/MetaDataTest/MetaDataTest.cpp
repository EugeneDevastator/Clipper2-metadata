#include <cstdlib>
#include <string>
#include <chrono>
#include <random>
#include <iostream>

#include "clipper2/clipper.h"
#include "../../Utils/clipper.svg.utils.h"
#include "../../Utils/ClipFileLoad.h"
#include "../../Utils/ClipFileSave.h"
#include "../../Utils/Timer.h"

using namespace Clipper2Lib;

const int display_width = 800, display_height = 600;

void DoMetadataTest();
void System(const std::string &filename);
void AddColoredPath(SvgWriter& svg, const Path64& path, unsigned int fill_color, unsigned int stroke_color);
void GetColorsForLoopId(int loop_id, unsigned int& fill_color, unsigned int& stroke_color);

int main()
{
  srand((unsigned)time(0));
  DoMetadataTest();
  return 0;
}

void AddColoredPath(SvgWriter& svg, const Path64& path, unsigned int fill_color, unsigned int stroke_color)
{
  svg.AddPath(path, false, FillRule::EvenOdd, fill_color, stroke_color, 2.0, true);
}

void GetColorsForLoopId(int loop_id, unsigned int& fill_color, unsigned int& stroke_color)
{
  switch(loop_id) {
    case 1:
      fill_color = 0x80FF6060;   // Light red
      stroke_color = 0xFFCC0000; // Dark red
      break;
    case 2:
      fill_color = 0x8060FF60;   // Light green
      stroke_color = 0xFF00CC00; // Dark green
      break;
    case 3:
      fill_color = 0x806060FF;   // Light blue
      stroke_color = 0xFF0000CC; // Dark blue
      break;
    default:
      fill_color = 0x80404040;   // Gray for unknown/intersection
      stroke_color = 0xFF000000; // Black
      break;
  }
}

void DoMetadataTest()
{
  std::cout << "=== Metadata Tracking Test ===\n";

  // Create two nested squares (outer and inner) - like a room with a hole
  Path64 outer_square;
  outer_square.push_back(Point64(100, 100, 0, 1));  // segment 0, sector 1
  outer_square.push_back(Point64(500, 100, 1, 1));  // segment 1, sector 1
  outer_square.push_back(Point64(500, 500, 2, 1));  // segment 2, sector 1
  outer_square.push_back(Point64(100, 500, 3, 1));  // segment 3, sector 1

  Path64 inner_square;
  inner_square.push_back(Point64(200, 200, 0, 2));  // segment 0, sector 2
  inner_square.push_back(Point64(400, 200, 1, 2));  // segment 1, sector 2
  inner_square.push_back(Point64(400, 400, 2, 2));  // segment 2, sector 2
  inner_square.push_back(Point64(200, 400, 3, 2));  // segment 3, sector 2

  // Create a cutting shape that intersects both
  Path64 cutter;
  cutter.push_back(Point64(250, 150, 0, 3));  // segment 0, sector 3
  cutter.push_back(Point64(550, 150, 1, 3));  // segment 1, sector 3
  cutter.push_back(Point64(550, 350, 2, 3));  // segment 2, sector 3
  cutter.push_back(Point64(250, 350, 3, 3));  // segment 3, sector 3

  std::cout << "Outer square: sector_id=1, 4 segments (0-3)\n";
  std::cout << "Inner square: sector_id=2, 4 segments (0-3)\n";
  std::cout << "Cutter shape: sector_id=3, 4 segments (0-3)\n\n";

  // Create the nested shape (outer - inner)
  Paths64 nested;
  nested.push_back(outer_square);
  nested.push_back(inner_square);

  // Perform intersection
  Clipper64 c64;
  c64.AddSubject(nested);
  c64.AddClip(Paths64{cutter});

  Paths64 solution;
  c64.Execute(ClipType::Intersection, FillRule::EvenOdd, solution);

  std::cout << "Result has " << solution.size() << " path(s)\n\n";

  // Analyze the result
  for (size_t p = 0; p < solution.size(); p++) {
    std::cout << "Path " << p << " (" << solution[p].size() << " vertices):\n";

    int intersections = 0;
    int from_sector1 = 0, from_sector2 = 0, from_sector3 = 0;

    for (size_t i = 0; i < solution[p].size(); i++) {
      const Point64& pt = solution[p][i];

      std::cout << "  v" << i << ": (" << pt.x << "," << pt.y << ")";

      if (pt.segment_id == -1) {
        std::cout << " [INTERSECTION]";
        intersections++;
      } else {
        std::cout << " [seg:" << pt.segment_id << " sector:" << pt.loop_id << "]";
        if (pt.loop_id == 1) from_sector1++;
        else if (pt.loop_id == 2) from_sector2++;
        else if (pt.loop_id == 3) from_sector3++;
      }
      std::cout << "\n";
    }

    std::cout << "\n  Summary: " << intersections << " intersections, "
              << from_sector1 << " from sector1, "
              << from_sector2 << " from sector2, "
              << from_sector3 << " from sector3\n\n";
  }

  // Create SVG visualization
  SvgWriter svg;

  // Show original shapes
  unsigned int fill, stroke;
  GetColorsForLoopId(1, fill, stroke);
  AddColoredPath(svg, outer_square, fill, stroke);

  GetColorsForLoopId(2, fill, stroke);
  AddColoredPath(svg, inner_square, fill, stroke);

  GetColorsForLoopId(3, fill, stroke);
  AddColoredPath(svg, cutter, fill, stroke);

  // Apply offset to solution paths for visual separation
  for (size_t p = 0; p < solution.size(); p++) {
    int path_loop_id = -1;

    // Get loop_id from first vertex
    if (!solution[p].empty()) {
      path_loop_id = solution[p][0].loop_id;
    }

    std::cout << "Solution path " << p << " has loop_id: " << path_loop_id << "\n";

    // Apply offset based on path index for visual separation
    double offset_distance = 10.0 + (p * 5.0);  // 10, 15, 20, etc.

    Paths64 offset_paths = InflatePaths(Paths64{solution[p]}, offset_distance, JoinType::Round, EndType::Polygon);

    GetColorsForLoopId(path_loop_id, fill, stroke);

    // Draw all offset results
    for (const Path64& offset_path : offset_paths) {
      AddColoredPath(svg, offset_path, fill, stroke);
    }
  }

  SvgAddCaption(svg, "Solution paths with offset for visual separation", 20, 20);
  SvgAddCaption(svg, "Red=ID1, Green=ID2, Blue=ID3, Black=Unknown(-1)", 20, 40);

  std::string filename = "metadata_test.svg";
  SvgSaveToFile(svg, filename, 800, 600, 20);

  std::cout << "Saved: " << filename << "\n";
  System(filename);
}

void System(const std::string& filename)
{
#ifdef _WIN32
  system(filename.c_str());
#else
  system(("firefox " + filename + " &").c_str());
#endif
}
