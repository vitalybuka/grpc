#include <signal.h>
#include <sys/time.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <thread>

#include <grpcpp/grpcpp.h>

using grpc::Channel;

void RunServer(int);
void RunClient(std::shared_ptr<Channel> channel);

void SignalHandler(int sig, siginfo_t* sinfo, void* ucontext) {}

void SetProf() {
  struct sigaction sa = {};
  sa.sa_sigaction = SignalHandler;

  sa.sa_flags = SA_RESTART | SA_SIGINFO;
  sigfillset(&sa.sa_mask);
  if (0 != sigaction(SIGPROF, &sa, nullptr)) abort();

  struct itimerval timer = {};
  timer.it_interval.tv_sec = 0;
  timer.it_interval.tv_usec = 10000;
  timer.it_value = timer.it_interval;
  setitimer(ITIMER_PROF, &timer, nullptr);
}

int main(int argc, char** argv) {
  int n = 56;
  int port = 53052;

  std::thread([&]() {
    std::atomic<std::uint64_t> c = {};

    for (int i = 0; i < n; ++i) {
      std::thread([i, &c, port]() {
        //std::cout << "new thread\n";
        sleep(10);

        for (;;) {
          std::string server_address = "127.0.0.1:" + std::to_string(port);
          auto channel = grpc::CreateChannel(
              server_address, grpc::InsecureChannelCredentials());
          RunClient(channel);
          ++c;
        }
      }).detach();
    }

    sigset_t set;
    sigfillset(&set);
    pthread_sigmask(SIG_BLOCK, &set, nullptr);

    time_t prevt = time(nullptr);
    for (;;) {
      sleep(10);
      time_t now = time(nullptr);
      std::cout << "Duration: " << (now - prevt) << " Requests: " << c << "\n";
      prevt = now;
      c = 0;
    }
  }).detach();

  SetProf();
  RunServer(port);

  return 0;
}
