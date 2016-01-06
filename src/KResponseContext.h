#ifndef KRESPONSECONTEXT_H
#define KRESPONSECONTEXT_H
#include "KBuffer.h"
#include "global.h"
#include <assert.h>
#ifdef ENABLE_ATOM
#include "katom.h"
#endif
#ifndef _WIN32
#include <sys/uio.h>
#endif
class KResponseContext
{
public:
	KResponseContext()
	{
		memset(this,0,sizeof(*this));
	}
	~KResponseContext()
	{
		clean();
	}
	void clean()
	{
		while (header_count>0) {
			--header_count;
			if (!header->skip_data_free) {
				xfree(header->data);
			}
			buff *next = header->next;
			xfree(header);
			header = next;
		}
		memset(this,0,sizeof(*this));
	}
	void getReadBuffer(LPWSABUF buffer,int &bufferCount)
	{
		assert(hot);
		int got = left;
		assert(header);
		buff *tmp = header;
		buffer[0].iov_base = hot;
		int hot_left = header->used - (hot - header->data);
		hot_left = MIN(hot_left,got);
		buffer[0].iov_len = hot_left;
		got -= hot_left;
		int i;
		for (i=1;i<bufferCount;i++) {
			tmp = tmp->next;
			if (tmp==NULL || got<=0) {
				break;
			}
			buffer[i].iov_base = tmp->data;
			buffer[i].iov_len = MIN(got,tmp->used);
			got -= buffer[i].iov_len;
		}
		bufferCount = i;
	}
	char *getReadBuffer(int &len)
	{
		WSABUF buffer;
		int bufferCount = 1;
		getReadBuffer(&buffer,bufferCount);
		len = buffer.iov_len;
		return (char *)buffer.iov_base;
	}
	bool readSuccess(int got)
	{
		left -= got;
		while (got>0) {
			int hot_left = header->used - (hot - header->data);
			int this_len = MIN(got,hot_left);
			hot += this_len;
			got -= this_len;
			if (header->used == hot - header->data) {
				buff *next = header->next;
				if (header_count>0) {
					--header_count;
					if (!header->skip_data_free) {
						xfree(header->data);
					}
					xfree(header);
				}
				header = next;
				if (header==NULL) {
					assert(got==0);
					assert(left==0);
					last = NULL;
					hot = NULL;
					return false;
				}
				hot = header->data;
			}
		}
		if (left>0) {
			return true;
		}
		clean();
		return false;
	}
	void append(const char *str,uint16_t len)
	{
		char *data = (char *)malloc(len);
		memcpy(data,str,len);
		pushEnd(data,len);
	}
	void pushEnd(buff *buf,bool isHead)
	{
		if (isHead) {
			++header_count;
		}
		assert(buf->skip_data_free==0);
		add(buf,buf->used);
	}
	void insert(const char *str,uint16_t len)
	{
		char *data = (char *)malloc(len);
		memcpy(data,str,len);
		pushHead(data,len);
	}
	void pushHead(char *str,uint16_t len)
	{
		if (header==NULL) {
			pushEnd(str,len);
			return;
		}
		buff *t = (buff *)xmalloc(sizeof(buff));
		memset(t,0,sizeof(buff));
		header_count++;
		t->data = str;
		t->used = len;
		t->next = header;
		assert(hot == header->data);
		header = t;
		hot = t->data;
		left += len;
	}
	void pushEnd(char *str,uint16_t len)
	{
		buff *t = (buff *)xmalloc(sizeof(buff));
		memset(t,0,sizeof(buff));
		header_count++;
		t->data = str;
		t->used = len;
		add(t,len);
	}
	void append(buff *buf,int start,int len)
	{
		while (buf) {
			if (start < buf->used) {
				if (start==0) {
					assert(buf->skip_data_free==0);
					add(buf,len);
				} else {
					buff *t = (buff *)xmalloc(sizeof(buff));
					memset(t,0,sizeof(buff));
					++header_count;
					t->skip_data_free = 1;
					t->data = buf->data + start;
					t->used = buf->used - start;
					t->next = buf->next;
					add(t,len);
				}
				break;
			}
			start -= buf->used;
			buf = buf->next;
		}
	}
	int getBufferSize()
	{
		return left;
	}
	buff *getHeader()
	{
		return header;
	}
private:
	void add(buff *buf,int len)
	{
		left += len;
		if (last==NULL) {
			assert(header==NULL);
			last = header = buf;
			hot = header->data;
		} else {
			last->next = buf;
			last = buf;
		}
	}
	buff *last;
	buff *header;
	char *hot;
	int left;
	int header_count;
};
#endif
