/*
 * Copyright 2016 Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
#include <folly/io/async/DelayedDestructionBase.h>

#include <functional>
#include <gtest/gtest.h>
#include <list>
#include <vector>

using namespace folly;

class DestructionOnCallback : public DelayedDestructionBase {
 public:
  DestructionOnCallback() : state_(0), deleted_(false) {
    onDestroy_ = [this] (bool delayed) {
      deleted_ = true;
      delete this;
      (void)delayed; // prevent unused variable warnings
    };
  }

  void onComplete(int n, int& state) {
    DestructorGuard dg(this);
    for (auto i = n; i >= 0; --i) {
      onStackedComplete(i);
    }
    state = state_;
  }

  void setOnDestroy(std::function<void(bool)> onDestroy) {
    onDestroy_ = onDestroy;
  }

  int state() const { return state_; }
  bool deleted() const { return deleted_; }

 protected:
  void onStackedComplete(int recur) {
    DestructorGuard dg(this);
    ++state_;
    if (recur <= 0) {
      return;
    }
    onStackedComplete(--recur);
  }
 private:
  int state_;
  bool deleted_;
};

struct DelayedDestructionBaseTest : public ::testing::Test {
};

TEST_F(DelayedDestructionBaseTest, basic) {
  DestructionOnCallback* d = new DestructionOnCallback();
  EXPECT_NE(d, nullptr);
  int32_t state;
  d->onComplete(3, state);
  EXPECT_EQ(state, 10); // 10 = 6 + 3 + 1
}

TEST_F(DelayedDestructionBaseTest, destructFromContainer) {
  std::list<DestructionOnCallback> l;
  l.emplace_back();
  l.back().setOnDestroy([&] (bool delayed) {
    l.erase(l.begin());
    (void)delayed;
  });
  EXPECT_NE(l.size(), 0);
  int32_t state;
  l.back().onComplete(3, state);
  EXPECT_EQ(state, 10); // 10 = 6 + 3 + 1
  EXPECT_EQ(l.size(), 0);
}
