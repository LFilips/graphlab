#include <graphlab/rpc/thread_local_send_buffer.hpp>
#include <graphlab/rpc/dc.hpp>

namespace graphlab {
namespace dc_impl {

thread_local_buffer::thread_local_buffer() {
  // allocate the buffers
  dc = distributed_control::get_instance();
  size_t nprocs = dc->numprocs(); 

  outbuf.resize(nprocs); 
  current_archive.resize(nprocs); 

  outbuf_locks.resize(nprocs);
  archive_locks.resize(nprocs);

  contended.clear();

  bytes_sent.resize(nprocs, 0);
  dc->register_send_buffer(this);
  procid = dc->procid();
}


thread_local_buffer::~thread_local_buffer() {
  dc->unregister_send_buffer(this);
  push_flush();
  // deallocate the buffers
  for (size_t i = 0; i < current_archive.size(); ++i) {
    if (current_archive[i].buf) {
      free(current_archive[i].buf);
      current_archive[i].buf = NULL;
    }
  }

  for (size_t i = 0; i < outbuf.size(); ++i) {
    for (size_t j = 0; j < outbuf[i].size(); ++j) {
      free(outbuf[i][j].first);
    }
  }
}

void thread_local_buffer::inc_calls_sent(procid_t target) {
  dc->inc_calls_sent(target);
}


void thread_local_buffer::push_flush() {
  for (size_t i = 0; i < outbuf.size(); ++i) {
    std::vector<std::pair<char*, size_t> >  buf = extract(i);
    for (size_t j = 0;j < buf.size(); ++j) {
      dc->write_to_buffer(i, buf[j].first, buf[j].second);
    }
    if (buf.size()) dc->flush_soon(i);
  }
}


void thread_local_buffer::pull_flush() {
  dc->flush();
}

void thread_local_buffer::pull_flush(procid_t p) {
  dc->flush(p);
}


void thread_local_buffer::pull_flush_soon() {
  dc->flush_soon();
}


void thread_local_buffer::pull_flush_soon(procid_t p) {
  dc->flush_soon(p);
}


}
}
