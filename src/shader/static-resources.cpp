#include <iostream>
#include <filesystem>
#include <fstream>
#include <span>

#include <logcerr/log.hpp>



namespace {
  std::string read_file(const std::filesystem::path& path) {
    std::ifstream input(path);
    if (!input) {
      throw std::runtime_error{"unable to open file " + path.string()};
    }

    std::string output = "  \"";
    size_t i = 0;
    while (input.peek() != -1) {
      if (++i % 20 == 0) {
        output += "\"\n  \"";
      }
      output += logcerr::format("\\x{:02x}", input.get());
    }
    output += "\"";
    return output;
  }



  std::string define_file(const std::filesystem::path& path, std::string_view id) {
    return logcerr::format("\n"
      "namespace {{ constexpr std::string_view content_{}_ {{\n"
      "{}\n"
      "}};}}\n"
      "\n"
      "std::string_view resources::{}() {{ return content_{}_; }}\n",
      id, read_file(path), id, id);
  }



  std::string declare_file(std::string_view id) {
    return logcerr::format("[[nodiscard]] std::string_view {}();\n", id);
  }
}





int main(int argc, const char* argv[]) {
  std::span<const char*> args{argv, static_cast<size_t>(argc)};

  if (args.size() < 3 || args.size() % 2 != 1) {
    std::cout << "usage: " << args[0]
      << " <header> <source> [<resource1 name>... <resource1>...]\n";
    return 1;
  }

  std::span names = args.subspan(3, (args.size() - 3) / 2);
  std::span files = args.subspan(3 + names.size());

  if (names.size() != files.size()) {
    throw std::runtime_error{"mismatch between input files and names"};
  }





  std::ofstream source(args[2]);
  if (!source) {
    throw std::runtime_error{std::string{"unable to write to "} + args[2]};
  }

  source << "#include " << std::filesystem::path{args[1]}.filename() << "\n";



  std::ofstream header(args[1]);
  if (!header) {
    throw std::runtime_error{std::string{"unable to write to "} + args[1]};
  }

  header << "// This file is generated during build. DO NOT EDIT.\n";
  header << "#pragma once\n";
  header << "#include <string_view>\n";
  header << "namespace resources {\n";



  for (size_t i = 0; i < names.size(); ++i) {
    header << declare_file(names[i]);
    source << define_file(files[i], names[i]);
  }

  header << "}\n";
}
