
#include <common.cxx>
#include <posix/fd.hpp>
#include <posix/fd_map.hpp>

class Test_fd : public FD {
public:
  Test_fd(const int id) : FD(id) {};

  ssize_t read(void*, size_t) override
  { return 1; }

  int close() override
  { return 0; }
};

class Hest_fd : public FD {
public:
  Hest_fd(const int id, std::string sound)
    : FD(id), sound_(std::move(sound))
  {}

  int close() override
  { return 0; }
private:
  std::string sound_;
};

CASE("Adding a implemented FD descriptor in FD_map")
{
  // Create
  auto& test = FD_map::_open<Test_fd>();

  // Unique ID
  auto& test2 = FD_map::_open<Test_fd>();
  EXPECT_NOT(test == test2);

  // Overriden function works
  const auto res = test.read(nullptr, 0);
  EXPECT(res == 1);

  // Get works
  const FD_map::id_t id = test.get_id();
  auto* get = FD_map::_get(id);
  EXPECT(get != nullptr);
  EXPECT(*get == test);

  // Close works
  FD_map::close(id);

  EXPECT(FD_map::_get(id) == nullptr);

  const int bet = 322; // this used to be a throw
  EXPECT(FD_map::_get(bet) == nullptr);
}
