#include <am-engine/http_utils.h>
#include <fds_assert.h>
#include <boost/tokenizer.hpp>
#include <sstream>

HttpRequest::HttpRequest(ngx_http_request_t* ngx_req) {
  fds_assert(ngx_req != NULL);

  _pReq = ngx_req;

  std::string uri((const char*) ngx_req->uri.data, ngx_req->uri.len);
  boost::char_separator<char> sep("/");
  boost::tokenizer<boost::char_separator<char>> tokens(uri, sep);
  for (const auto& t : tokens) {
    _uriParts.push_back(t);
  }
}

std::vector<std::string>& HttpRequest::getURIParts()
{
  return _uriParts;
}
const std::vector<std::string>& HttpRequest::getURIParts() const
{
  return _uriParts;
}

ngx_http_request_t* HttpRequest::getNginxReq() const
{
  return _pReq;
}

std::string HttpRequest::toString() const
{
  std::ostringstream stream;
  stream << "Method: " << std::string((char*)_pReq->method_name.data, _pReq->method_name.len);
  stream << "\nUri parts: ";
  for (int i = 0; i < _uriParts.size(); i++) {
    stream << _uriParts[i] << " ";
  }
//  stream << "\nContent len: " << _pReq->
  return stream.str();
}
