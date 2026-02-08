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
  std::cout << "=== Two-Step Metadata Test ===\n";

  // Step 1: Create outer square (sector 1)
  Path64 outer_square;
  outer_square.push_back(Point64(100, 100, 0, 1));
  outer_square.push_back(Point64(500, 100, 1, 1));
  outer_square.push_back(Point64(500, 500, 2, 1));
  outer_square.push_back(Point64(100, 500, 3, 1));

  // Inner square to cut hole (sector 2)
  Path64 inner_square;
  inner_square.push_back(Point64(200, 200, 0, 2));
  inner_square.push_back(Point64(400, 200, 1, 2));
  inner_square.push_back(Point64(400, 400, 2, 2));
  inner_square.push_back(Point64(200, 400, 3, 2));

  std::cout << "Step 1: Cut hole in red square\n";
  std::cout << "Outer square: sector_id=1\n";
  std::cout << "Inner square: sector_id=2\n\n";

  // Step 1: Cut hole - difference operation
  Clipper64 c1;
  c1.AddSubject(Paths64{outer_square});
  c1.AddClip(Paths64{inner_square});

  Paths64 donut_result;
  c1.Execute(ClipType::Difference, FillRule::NonZero, donut_result);

  std::cout << "Donut result has " << donut_result.size() << " path(s)\n";

  // Step 2: Create cutting rectangle (sector 3)
  Path64 cutter_rect;
  cutter_rect.push_back(Point64(250, 50, 0, 3));
  cutter_rect.push_back(Point64(450, 50, 1, 3));
  cutter_rect.push_back(Point64(450, 350, 2, 3));
  cutter_rect.push_back(Point64(250, 350, 3, 3));

  std::cout << "\nStep 2: Cut both donut and inner square with rectangle\n";
  std::cout << "Cutter rectangle: sector_id=3\n\n";

  // Combine donut and inner square for final cut
  Paths64 combined_shapes = donut_result;
  combined_shapes.push_back(inner_square);

  // Step 2: Cut everything with rectangle
  Clipper64 c2;
  c2.AddSubject(combined_shapes);
  c2.AddClip(Paths64{cutter_rect});

  Paths64 final_result;
  c2.Execute(ClipType::Intersection, FillRule::NonZero, final_result);

  std::cout << "Final result has " << final_result.size() << " path(s)\n\n";

  // Analyze final result
  for (size_t p = 0; p < final_result.size(); p++) {
    std::cout << "Final path " << p << " (" << final_result[p].size() << " vertices):\n";

    for (size_t i = 0; i < final_result[p].size(); i++) {
      const Point64& pt = final_result[p][i];
      std::cout << "  v" << i << ": (" << pt.x << "," << pt.y << ")";

      if (pt.segment_id == -1) {
        std::cout << " [INTERSECTION]";
      } else {
        std::cout << " [seg:" << pt.segment_id << " sector:" << pt.loop_id << "]";
      }
      std::cout << "\n";
    }
    std::cout << "\n";
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
  AddColoredPath(svg, cutter_rect, fill, stroke);

  // Show intermediate donut result (lighter colors)
  for (const Path64& donut_path : donut_result) {
    int path_loop_id = donut_path.empty() ? -1 : donut_path[0].loop_id;
    GetColorsForLoopId(path_loop_id, fill, stroke);
    fill = (fill & 0xFF000000) | ((fill & 0x00FFFFFF) >> 1); // Make lighter
    AddColoredPath(svg, donut_path, fill, stroke);
  }

  // Apply inward offset to final result paths for visual separation
  for (size_t p = 0; p < final_result.size(); p++) {
    int path_loop_id = final_result[p].empty() ? -1 : final_result[p][0].loop_id;

    std::cout << "Final path " << p << " has loop_id: " << path_loop_id << "\n";

    // Apply negative offset (inward) with square joins
    double offset_distance = -(8.0 + (p * 4.0));

    Paths64 offset_paths = InflatePaths(Paths64{final_result[p]}, offset_distance, JoinType::Square, EndType::Polygon);

    GetColorsForLoopId(path_loop_id, fill, stroke);

    // Draw all offset results
    for (const Path64& offset_path : offset_paths) {
      AddColoredPath(svg, offset_path, fill, stroke);
    }
  }

  SvgAddCaption(svg, "Two-step operation: 1) Cut hole 2) Cut with rectangle", 20, 20);
  SvgAddCaption(svg, "Red=Sector1, Green=Sector2, Blue=Cutter", 20, 40);

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
