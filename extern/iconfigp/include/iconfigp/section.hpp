#ifndef ICONFIGP_SECTION_HPP_INCLUDED
#define ICONFIGP_SECTION_HPP_INCLUDED

#include "iconfigp/exception.hpp"
#include "iconfigp/group.hpp"
#include "iconfigp/opt-ref.hpp"

#include <algorithm>
#include <iterator>
#include <numeric>
#include <span>
#include <string_view>
#include <vector>



namespace iconfigp {

class section {
  friend class parser;

  public:
    [[nodiscard]] size_t offset() const { return offset_; }

    [[nodiscard]] std::string_view name() const {
      used_ = true;
      return name_;
    };





    [[nodiscard]] std::span<const group> groups() const {
      used_ = true;
      return groups_;
    }





    [[nodiscard]] std::span<const section> subsections() const {
      used_ = true;
      return sections_;
    }



    [[nodiscard]] opt_ref<const section> subsection(std::string_view name) const {
      used_ = true;

      auto it = std::ranges::find_if(sections_,
          [name](const auto& sect) { return sect.name() == name; });

      if (it != sections_.end()) {
        return *it;
      }
      return {};
    }





    [[nodiscard]] opt_ref<const key_value> unique_key(std::string_view name) const {
      used_ = true;

      opt_ref<const key_value> output;

      for (const auto& grp: groups_) {
        if (auto kv = grp.unique_key(name)) {
          if (output) {
            throw multiple_definitions_exception{*output, *kv, true};
          }
          output = kv;
        }
      }

      return output;
    }



    [[nodiscard]] const key_value& require_unique_key(std::string_view name) const {
      used_ = true;

      if (auto ref = unique_key(name)) {
        return *ref;
      }
      throw missing_key_exception{std::string{name}, offset_};
    }



    [[nodiscard]] size_t count_keys(std::string_view name) const {
      used_ = true;

      return std::transform_reduce(groups_.begin(), groups_.end(), 0, std::plus{},
          [name](const auto& gp) { return gp.count_keys(name); });
    }





    [[nodiscard]] bool used() const { return used_; }



    [[nodiscard]] std::vector<const key_value*> unused_keys() const {
      std::vector<const key_value*> output;

      if (!used_) {
        return output;
      }

      for (const auto& gp: groups_) {
        for (const auto& kv: gp.values_) {
          if (!kv.used()) {
            output.emplace_back(&kv);
          }
        }
      }

      for (const auto& sec: sections_) {
        std::ranges::copy(sec.unused_keys(), std::back_inserter(output));
      }

      return output;
    }



    [[nodiscard]] std::vector<const section*> unused_sections() const {
      if (!used_) {
        return {this};
      }

      std::vector<const section*> output;
      for (const auto& sec: sections_) {
        std::ranges::copy(sec.unused_sections(), std::back_inserter(output));
      }
      return output;
    }





  private:
    std::string          name_;
    size_t               offset_;

    std::vector<section> sections_;
    size_t               current_section_{0};

    std::vector<group>   groups_;

    mutable bool         used_{false};





    section(std::string name, size_t offset) :
      name_  {std::move(name)},
      offset_{offset}
    {
      groups_.push_back(group{offset_});
    }





    group& current_group() {
      if (current_section_ < sections_.size()) {
        return sections_[current_section_].current_group();
      }
      return groups_.back();
    }



    void new_group(size_t offset) {
      if (current_section_ < sections_.size()) {
        sections_[current_section_].new_group(offset);
      } else {
        if (groups_.back().empty()) {
          groups_.pop_back();
        }
        groups_.push_back(group{offset});
      }
    }





    template<typename... Args>
    void append(Args&&... args) {
      current_group().append(std::forward<Args>(args)...);
    }





    void select_section(std::span<std::string> path, size_t offset) {
      if (path.empty() || path.front().empty()) {
        current_section_ = sections_.size();
        new_group(offset);
        return;
      }

      if (auto it = std::ranges::find_if(sections_,
            [path](const auto& section) { return section.name_ == path.front(); });
          it != sections_.end()
      ) {
        current_section_ = it - sections_.begin();
        it->select_section(path.subspan(1), offset);
      } else {
        current_section_ = sections_.size();
        sections_.push_back(section{std::move(path.front()), offset});
        sections_.back().select_section(path.subspan(1), offset);
      }
    }

};



[[nodiscard]] std::optional<std::string> generate_unused_message(const section&,
    std::string_view source = "", bool colored = false);

}

#endif // ICONFIGP_SECTION_HPP_INCLUDED
