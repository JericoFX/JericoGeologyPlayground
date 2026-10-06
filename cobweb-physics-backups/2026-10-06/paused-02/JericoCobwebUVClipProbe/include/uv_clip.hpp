#pragma once
#include <cobweb.hpp>
#include <cstddef>
#include <stdexcept>
namespace web::uvclip {
using ConvexCut = std::vector<V2>;
using Barycentric = std::array<double,3>;
struct Limits { std::size_t maxTriangles=8400, maxVertices=25200, maxWorkingPolygons=100000; };
enum class ErrorCode { InvalidInput, BudgetExceeded, NumericalFailure };
class ClipError : public std::runtime_error {
public:
    ClipError(ErrorCode code, const std::string& message):std::runtime_error(message),code(code){}
    ErrorCode code;
};
struct Result {
    Mesh mesh;
    std::vector<std::size_t> sourceFaces;
    std::vector<std::array<Barycentric,3>> sourceBarycentrics;
    double sourceUVArea=0, retainedUVArea=0;
    std::size_t affectedSourceFaces=0;
};
// Transactional: source is never modified; throws ClipError on invalid input,
// unsupported numerical geometry or exceeded output/working budgets.
Result subtract(const Mesh& source,const std::vector<ConvexCut>& cuts,const Limits& limits={});
}
