#include "jack_client.h"

USING_PART_OF_NAMESPACE_EIGEN

inline void process(const Eigen::MatrixXd &in, Eigen::MatrixXd &out)  
{
  out = 2 * in;
}

int main() {
  ladtl::jack_client<> client("test_jack_client", process, ladtl::shutdown_handler<ladtl::jack_client<> >);
  client();
}
