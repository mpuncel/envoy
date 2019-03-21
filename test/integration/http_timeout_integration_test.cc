#include "test/integration/http_timeout_integration_test.h"

#include "gtest/gtest.h"

namespace Envoy {

INSTANTIATE_TEST_SUITE_P(IpVersions, HttpTimeoutIntegrationTest,
                         testing::ValuesIn(TestEnvironment::getIpVersionsForTest()),
                         TestUtility::ipTestParamsToString);

// Sends a request with a global timeout specified, sleeps for longer than the
// timeout, and ensures that a timeout is received.
TEST_P(HttpTimeoutIntegrationTest, GlobalTimeout) {
  initialize();

  std::cout << "here1" << std::endl;
  codec_client_ = makeHttpConnection(makeClientConnection(lookupPort("http")));
  std::cout << "here2" << std::endl;
  auto encoder_decoder =
      codec_client_->startRequest(Http::TestHeaderMapImpl{{":method", "POST"},
                                                          {":path", "/test/long/url"},
                                                          {":scheme", "http"},
                                                          {":authority", "host"},
                                                          {"x-forwarded-for", "10.0.0.1"},
                                                          {"x-envoy-upstream-rq-timeout-ms", "500"}});
  std::cout << "here3" << std::endl;
  auto response = std::move(encoder_decoder.second);
  request_encoder_ = &encoder_decoder.first;
  std::cout << "here4" << std::endl;

  ASSERT_TRUE(fake_upstreams_[0]->waitForHttpConnection(*dispatcher_, fake_upstream_connection_));
  ASSERT_TRUE(fake_upstream_connection_->waitForNewStream(*dispatcher_, upstream_request_));
  ASSERT_TRUE(upstream_request_->waitForHeadersComplete());
  codec_client_->sendData(*request_encoder_, 0, true);

  ASSERT_TRUE(upstream_request_->waitForEndStream(*dispatcher_));
  timeSystem().sleep(std::chrono::milliseconds(501));
  response->waitForHeaders();
  ASSERT_TRUE(upstream_request_->waitForReset(std::chrono::milliseconds(0)));

  codec_client_->close();

  EXPECT_TRUE(upstream_request_->complete());
  EXPECT_EQ(0U, upstream_request_->bodyLength());

  EXPECT_TRUE(response->complete());
  EXPECT_STREQ("504", response->headers().Status()->value().c_str());
}

} // namespace Envoy
