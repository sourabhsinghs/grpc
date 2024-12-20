// Copyright 2024 gRPC authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef GRPC_SRC_CORE_EXT_TRANSPORT_CHAOTIC_GOOD_CLIENT_CHAOTIC_GOOD_CONNECTOR_H
#define GRPC_SRC_CORE_EXT_TRANSPORT_CHAOTIC_GOOD_CLIENT_CHAOTIC_GOOD_CONNECTOR_H

#include <grpc/event_engine/event_engine.h>
#include <grpc/support/port_platform.h>

#include <cstddef>
#include <cstdint>
#include <memory>

#include "absl/random/random.h"
#include "absl/status/statusor.h"
#include "src/core/client_channel/connector.h"
#include "src/core/ext/transport/chaotic_good/config.h"
#include "src/core/handshaker/handshaker.h"
#include "src/core/lib/channel/channel_args.h"
#include "src/core/lib/event_engine/channel_args_endpoint_config.h"
#include "src/core/lib/iomgr/closure.h"
#include "src/core/lib/iomgr/endpoint.h"
#include "src/core/lib/iomgr/error.h"
#include "src/core/lib/iomgr/exec_ctx.h"
#include "src/core/lib/promise/activity.h"
#include "src/core/lib/promise/inter_activity_latch.h"
#include "src/core/lib/promise/wait_for_callback.h"
#include "src/core/lib/resource_quota/arena.h"
#include "src/core/lib/resource_quota/memory_quota.h"
#include "src/core/lib/resource_quota/resource_quota.h"
#include "src/core/lib/transport/promise_endpoint.h"
#include "src/core/util/notification.h"
#include "src/core/util/ref_counted_ptr.h"
#include "src/core/util/sync.h"

namespace grpc_core {
namespace chaotic_good {
class ChaoticGoodConnector : public SubchannelConnector {
 public:
  explicit ChaoticGoodConnector(
      std::shared_ptr<grpc_event_engine::experimental::EventEngine>
          event_engine);
  ~ChaoticGoodConnector() override;
  void Connect(const Args& args, Result* result, grpc_closure* notify) override;
  void Shutdown(grpc_error_handle error) override {
    ActivityPtr connect_activity;
    MutexLock lock(&mu_);
    if (is_shutdown_) return;
    is_shutdown_ = true;
    if (handshake_mgr_ != nullptr) {
      handshake_mgr_->Shutdown(error);
    }
    connect_activity = std::move(connect_activity_);
  };

 private:
  static auto DataEndpointReadSettingsFrame(
      RefCountedPtr<ChaoticGoodConnector> self, uint32_t data_connection_index);
  static auto DataEndpointWriteSettingsFrame(
      RefCountedPtr<ChaoticGoodConnector> self, uint32_t data_connection_index);
  static auto ControlEndpointReadSettingsFrame(
      RefCountedPtr<ChaoticGoodConnector> self);
  static auto ControlEndpointWriteSettingsFrame(
      RefCountedPtr<ChaoticGoodConnector> self);
  static auto WaitForDataEndpointSetup(RefCountedPtr<ChaoticGoodConnector> self,
                                       uint32_t data_connection_index);
  void OnHandshakeDone(absl::StatusOr<HandshakerArgs*> result);

  RefCountedPtr<Arena> arena_ = SimpleArenaAllocator()->MakeArena();
  Mutex mu_;
  Args args_;
  std::unique_ptr<chaotic_good::Config> config_;
  Result* result_ ABSL_GUARDED_BY(mu_);
  grpc_closure* notify_ ABSL_GUARDED_BY(mu_) = nullptr;
  bool is_shutdown_ ABSL_GUARDED_BY(mu_) = false;
  absl::StatusOr<grpc_event_engine::experimental::EventEngine::ResolvedAddress>
      resolved_addr_;

  PromiseEndpoint control_endpoint_;
  std::vector<PromiseEndpoint> data_endpoints_;
  std::vector<std::string> connection_ids_;
  ActivityPtr connect_activity_ ABSL_GUARDED_BY(mu_);
  const std::shared_ptr<grpc_event_engine::experimental::EventEngine>
      event_engine_;
  RefCountedPtr<HandshakeManager> handshake_mgr_;
  std::vector<std::unique_ptr<InterActivityLatch<void>>> data_endpoint_ready_;
};
}  // namespace chaotic_good
}  // namespace grpc_core

grpc_channel* grpc_chaotic_good_channel_create(const char* target,
                                               const grpc_channel_args* args);

#endif  // GRPC_SRC_CORE_EXT_TRANSPORT_CHAOTIC_GOOD_CLIENT_CHAOTIC_GOOD_CONNECTOR_H
