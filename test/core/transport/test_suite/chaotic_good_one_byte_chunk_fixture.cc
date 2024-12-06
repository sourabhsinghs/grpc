// Copyright 2023 gRPC authors.
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

#include "test/core/transport/test_suite/chaotic_good_fixture_helpers.h"

namespace grpc_core {

TRANSPORT_FIXTURE(ChaoticGoodOneByteChunks) {
  auto resource_quota = MakeResourceQuota("test");
  EndpointPair control_endpoints =
      CreateEndpointPair(event_engine.get(), resource_quota.get(), 1234);
  EndpointPair data_endpoints =
      CreateEndpointPair(event_engine.get(), resource_quota.get(), 4321);
  auto channel_args =
      ChannelArgs()
          .SetObject(resource_quota)
          .SetObject(
              std::static_pointer_cast<
                  grpc_event_engine::experimental::EventEngine>(event_engine));
  chaotic_good::Config config(channel_args);
  config.TestOnlySetChunkSizes(1);
  auto client_transport =
      MakeOrphanable<chaotic_good::ChaoticGoodClientTransport>(
          std::move(control_endpoints.client), std::vector<PromiseEndpoint>{},
          ChannelArgs().SetObject(resource_quota), event_engine, config);
  auto server_transport =
      MakeOrphanable<chaotic_good::ChaoticGoodServerTransport>(
          channel_args, std::move(control_endpoints.server),
          std::vector<PromiseEndpoint>{}, event_engine, config);
  return ClientAndServerTransportPair{std::move(client_transport),
                                      std::move(server_transport), true};
}

}  // namespace grpc_core