从生产服务器下载行情数据和日志到云服务器上。


加密：
tar -czvf - file | openssl des3 -salt -k password -out /path/to/file.tar.gz

解密：
openssl des3 -d -k password -salt -in /path/to/file.tar.gz | tar xzf -:


例子：
tar -czvf - -C ./ yao_download_night_data.sh | openssl des3 -salt -k 617999 -out ./yao_download_night_data.txt 