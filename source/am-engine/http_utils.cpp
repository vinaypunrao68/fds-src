#include <am-engine/http_utils.h>
#include <fds_assert.h>
#include <boost/tokenizer.hpp>
#include <sstream>

std::string HttpUtils::computeEtag(const char* data, size_t len)
{
    ngx_md5_t ctx;
    unsigned char result[16];
    char md5string[34];
    ngx_md5_init(&ctx);
    ngx_md5_update(&ctx, data, len);
    ngx_md5_final(result, &ctx);

    for(int i = 0; i < 16; ++i)
        sprintf(&md5string[i*2], "%02x", (unsigned int)result[i]);

    md5string[33] = '\0';
    return std::string((const char*)md5string);
}

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

// getReqHdrVal
// --------------------
// Return the value corresponding with the key in the request header.
//
fds_bool_t HttpRequest::getReqHdrVal(char const *const key,
                                     std::string& data) const {
  ngx_list_t ngxList = _pReq->headers_in.headers;

  /*
   * Iterate over each element in each part.
   */
  for(ngx_list_part_t *part = &(ngxList.part); part != NULL; part = part->next) {
    for (fds_uint32_t nelts = 0; nelts < part->nelts; nelts++) {
      ngx_table_elt_t *elt = &(((ngx_table_elt_t *)part->elts)[nelts]);
      ngx_str_t eltKey = elt->key;

      /*
       * Check if key matches input.
       * Note: We're only comparing with eltKey's length so
       * if the input key is a super-string of eltKey it will
       * still match.
       */
      if (strncmp(key, (const char *)eltKey.data, eltKey.len) == 0) {
        ngx_str_t eltData = elt->value;
        data.assign((const char *)eltData.data, eltData.len);
        return true;
      }
    }
  } 

  return false;
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
