#!/bin/bash

#curl -v -X PUT http://localhost:8000/fdspanda
#curl -v -X PUT http://localhost:8000/fdslemur
#curl -v -X PUT http://localhost:8000/fdsfrog
#curl -v -X PUT http://localhost:8000/fdsmuskrat
curl -v -X POST http://localhost:7777/api/config/volumes -d '{"name":"fdspanda","data_connector":{"type":"OBJECT","api":"S3, Swift"},"priority":10,"sla":0,"limit":300,"commit_log_retention":86400}'
curl -v -X POST http://localhost:7777/api/config/volumes -d '{"name":"fdslemur","data_connector":{"type":"OBJECT","api":"S3, Swift"},"priority":10,"sla":0,"limit":300,"commit_log_retention":86400}'
curl -v -X POST http://localhost:7777/api/config/volumes -d '{"name":"fdsfrog","data_connector":{"type":"OBJECT","api":"S3, Swift"},"priority":10,"sla":0,"limit":300,"commit_log_retention":86400}'
curl -v -X POST http://localhost:7777/api/config/volumes -d '{"name":"fdsmuskrat","data_connector":{"type":"OBJECT","api":"S3, Swift"},"priority":10,"sla":0,"limit":300,"commit_log_retention":86400}'
