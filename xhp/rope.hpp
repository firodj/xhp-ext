#include "rope.h"

class jrope {
  public:
    jrope(): s_(0) {
      r_ = rope_new();
    }
    jrope(const jrope &jr): s_(0) {
      r_ = rope_copy(jr.r_);
    }
    jrope(const char *str): s_(0) {
      r_ = rope_new_with_utf8((uint8_t*)str);
    }
    jrope(size_t num, char c): s_(0) {
      char tmp[2] = {c, 0};
      r_ = rope_new();
      while (num) { append((const char*)tmp); num--; };
    }
    const char *c_str() {
      if (s_) r_->free(s_);
      s_ = create_cstr();
      return (const char*)s_;
    }
    void insert(size_t pos, const char* str) {
      ROPE_RESULT result = rope_insert(r_, pos, (uint8_t *)str);
    }
    void append(const char* str) {
      insert(length(), str);
    }
    size_t length() const
    {
      return rope_char_count(r_);
    }

    jrope& operator=(const jrope &jr) {
      if (r_) rope_free(r_);
      r_ = rope_copy(jr.r_);
      return *this;
    }
    jrope& operator=(const char* str) {
      if (r_) rope_free(r_);
      r_ = rope_new_with_utf8((uint8_t*)str);
      return *this;
    }
    jrope& operator+=(const jrope &jr) {
      if (s_) r_->free(s_);
      s_ = jr.create_cstr();
      append((const char*)s_);
      return *this;
    }
    jrope& operator+=(const char* right) {
      append(right);
      return *this;
    }
    jrope operator+(const char* right) const {
      jrope res(*this);
      res += right;
      return res;
    }
    jrope operator+(const jrope &jr) const {
      jrope res(*this);
      res += jr;
      return res;
    }
    bool operator==(const jrope &jr) const {
      rope_node *iter1 = &(r_)->head;
      rope_node *iter2 = &(jr.r_)->head;

      size_t pos1 = 0;
      size_t pos2 = 0;

      while (iter1 != NULL && iter2 != NULL) {
        if (*(rope_node_data(iter1)+pos1) != *(rope_node_data(iter2)+pos2)) return false;

        pos1++;
        if (pos1 >= rope_node_num_bytes(iter1)) {
          iter1 = iter1->nexts[0].node;
          pos1 = 0;
        }

        pos2++;
        if (pos2 >= rope_node_num_bytes(iter2)) {
          iter2 = iter2->nexts[0].node;
          pos2 = 0;
        }
      }

      return iter1 == NULL && iter2 == NULL;
    }
    void clear()
    {
      if (r_) rope_free(r_);
      r_ = rope_new();
    }

    ~jrope() {
      rope_free(r_);
      if (s_) r_->free(s_);
    }

    void erase(size_t pos, size_t num)
    {
      rope_del(r_, pos, num);
    }
    bool empty() const {
      return length() == 0;
    }
    void replace(size_t pos, size_t num, const char *str)
    {
      erase(pos, num);
      insert(pos, str);
    }
    void replace(size_t pos, size_t num, const jrope &jr)
    {
      if (s_) r_->free(s_);
      s_ = jr.create_cstr();
      replace(pos, num, (const char*)s_);
    }

  protected:
    uint8_t *create_cstr() const {
      return rope_create_cstr(r_);
    }

    rope *r_;
    uint8_t *s_;
};
