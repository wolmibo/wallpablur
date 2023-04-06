#ifndef WALLPABLUR_WM_CHANGE_TOKEN_HPP_INCLUDED
#define WALLPABLUR_WM_CHANGE_TOKEN_HPP_INCLUDED

#include <concepts>
#include <memory>
#include <mutex>



template<std::regular T>
class change_token {
  template<std::regular U> friend class change_source;

  public:
    change_token() :
      state_{std::make_shared<shared_state>()}
    {}



    class locked_ref {
      public:
        locked_ref(T& ref, std::unique_lock<std::mutex>&& lock) :
          ref_ {&ref},
          lock_{std::move(lock)}
        {}


        const T& operator*() const { return *ref_; }
              T& operator*()       { return *ref_; }

        const T* operator->() const { return ref_; }
              T* operator->()       { return ref_; }


      private:
        T*                           ref_;
        std::unique_lock<std::mutex> lock_;
    };



    [[nodiscard]] bool changed() const {
      std::lock_guard lock{state_->mutex};

      return state_->changed;
    }



    [[nodiscard]] locked_ref get() const {
      std::unique_lock lock{state_->mutex};

      state_->changed = false;

      return locked_ref{state_->value, std::move(lock)};
    }



  private:
    struct shared_state {
      std::mutex mutex;
      bool       changed{true};
      T          value;
    };

    std::shared_ptr<shared_state> state_;

    explicit change_token(const std::shared_ptr<shared_state> state) : state_{state} {}
};





template<std::regular T>
class change_source {
  public:
    change_source() :
      state_{std::make_shared<typename change_token<T>::shared_state>()}
    {}



    [[nodiscard]] change_token<T> create_token() const {
      std::lock_guard lock{state_->mutex};

      state_->changed = true;

      return change_token<T>{state_};
    }



    void set(T&& value) {
      std::lock_guard lock{state_->mutex};

      state_->changed = state_->changed || (state_->value != value);
      state_->value   = std::move(value);
    }



  private:
    std::shared_ptr<typename change_token<T>::shared_state> state_;
};

#endif // WALLPABLUR_WM_CHANGE_TOKEN_HPP_INCLUDED
