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
  std::cout << "=== Three-Step Metadata Test ===\n";

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

  // Step 1.5: Shrink donut and inner square
  std::cout << "\nStep 1.5: Shrink both shapes\n";

  Paths64 shrunken_donut = InflatePaths(donut_result, -5.0, JoinType::Square, EndType::Polygon);
  Paths64 shrunken_inner = InflatePaths(Paths64{inner_square}, -5.0, JoinType::Square, EndType::Polygon);

  std::cout << "Shrunken donut has " << shrunken_donut.size() << " path(s)\n";
  std::cout << "Shrunken inner has " << shrunken_inner.size() << " path(s)\n";

  // Step 2: Create cutting rectangle (sector 3)
  Path64 cutter_rect;
  cutter_rect.push_back(Point64(250, 50, 0, 3));
  cutter_rect.push_back(Point64(450, 50, 1, 3));
  cutter_rect.push_back(Point64(450, 350, 2, 3));
  cutter_rect.push_back(Point64(250, 350, 3, 3));

  std::cout << "\nStep 2: Cut donut and inner square separately with rectangle\n";
  std::cout << "Cutter rectangle: sector_id=3\n\n";

  // Step 2a: Cut shrunken donut with rectangle
  Clipper64 c2a;
  c2a.AddSubject(shrunken_donut);
  c2a.AddClip(Paths64{cutter_rect});

  Paths64 cut_donut_result;
  c2a.Execute(ClipType::Intersection, FillRule::NonZero, cut_donut_result);

  // Step 2b: Cut shrunken inner square with rectangle
  Clipper64 c2b;
  c2b.AddSubject(shrunken_inner);
  c2b.AddClip(Paths64{cutter_rect});

  Paths64 cut_inner_result;
  c2b.Execute(ClipType::Intersection, FillRule::NonZero, cut_inner_result);

  std::cout << "Cut donut result has " << cut_donut_result.size() << " path(s)\n";
  std::cout << "Cut inner result has " << cut_inner_result.size() << " path(s)\n";

  // Step 3: Final offset on all results
  std::cout << "\nStep 3: Final offset on all cut results\n";

  Paths64 final_donut = InflatePaths(cut_donut_result, -3.0, JoinType::Square, EndType::Polygon);
  Paths64 final_inner = InflatePaths(cut_inner_result, -3.0, JoinType::Square, EndType::Polygon);

  std::cout << "Final donut has " << final_donut.size() << " path(s)\n";
  std::cout << "Final inner has " << final_inner.size() << " path(s)\n\n";

  // Analyze results
  std::cout << "=== Final Donut Pieces ===\n";
  for (size_t p = 0; p < final_donut.size(); p++) {
    std::cout << "Donut path " << p << " (" << final_donut[p].size() << " vertices):\n";
    for (size_t i = 0; i < final_donut[p].size(); i++) {
      const Point64& pt = final_donut[p][i];
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

  std::cout << "=== Final Inner Pieces ===\n";
  for (size_t p = 0; p < final_inner.size(); p++) {
    std::cout << "Inner path " << p << " (" << final_inner[p].size() << " vertices):\n";
    for (size_t i = 0; i < final_inner[p].size(); i++) {
      const Point64& pt = final_inner[p][i];
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

  // Show original shapes (very light)
  unsigned int fill, stroke;
  GetColorsForLoopId(1, fill, stroke);
  fill = (fill & 0xFF000000) | ((fill & 0x00FFFFFF) >> 3); // Very light
  AddColoredPath(svg, outer_square, fill, stroke);

  GetColorsForLoopId(2, fill, stroke);
  fill = (fill & 0xFF000000) | ((fill & 0x00FFFFFF) >> 3); // Very light
  AddColoredPath(svg, inner_square, fill, stroke);

  GetColorsForLoopId(3, fill, stroke);
  fill = (fill & 0xFF000000) | ((fill & 0x00FFFFFF) >> 3); // Very light
  AddColoredPath(svg, cutter_rect, fill, stroke);

  // Show intermediate donut result (medium opacity)
  for (const Path64& path : donut_result) {
    int path_loop_id = path.empty() ? -1 : path[0].loop_id;
    GetColorsForLoopId(path_loop_id, fill, stroke);
    fill = (fill & 0xFF000000) | ((fill & 0x00FFFFFF) >> 1); // Medium opacity
    AddColoredPath(svg, path, fill, stroke);
  }

  // Show shrunken shapes (lighter)
  for (const Path64& path : shrunken_donut) {
    int path_loop_id = path.empty() ? -1 : path[0].loop_id;
    GetColorsForLoopId(path_loop_id, fill, stroke);
    fill = (fill & 0xFF000000) | ((fill & 0x00FFFFFF) >> 1); // Lighter
    stroke = (stroke & 0xFF000000) | ((stroke & 0x00FFFFFF) >> 1);
    AddColoredPath(svg, path, fill, stroke);
  }

  for (const Path64& path : shrunken_inner) {
    int path_loop_id = path.empty() ? -1 : path[0].loop_id;
    GetColorsForLoopId(path_loop_id, fill, stroke);
    fill = (fill & 0xFF000000) | ((fill & 0x00FFFFFF) >> 1); // Lighter
    stroke = (stroke & 0xFF000000) | ((stroke & 0x00FFFFFF) >> 1);
    AddColoredPath(svg, path, fill, stroke);
  }

  // Show final results with full colors
  for (const Path64& path : final_donut) {
    int path_loop_id = path.empty() ? -1 : path[0].loop_id;
    GetColorsForLoopId(path_loop_id, fill, stroke);
    AddColoredPath(svg, path, fill, stroke);
  }

  for (const Path64& path : final_inner) {
    int path_loop_id = path.empty() ? -1 : path[0].loop_id;
    GetColorsForLoopId(path_loop_id, fill, stroke);
    AddColoredPath(svg, path, fill, stroke);
  }

  SvgAddCaption(svg, "Three-step: 1) Cut hole 2) Shrink 3) Cut separately 4) Final shrink", 20, 20);
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
