从生产服务器下载行情数据和日志到云服务器上。


靠靠
tar -czvf - file | openssl des3 -salt -k password -out /path/to/file.tar.gz

靠靠
openssl des3 -d -k password -salt -in /path/to/file.tar.gz | tar xzf -
