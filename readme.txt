�����������������������ݺ���־���Ʒ������ϡ�


����
tar -czvf - file | openssl des3 -salt -k password -out /path/to/file.tar.gz

����
openssl des3 -d -k password -salt -in /path/to/file.tar.gz | tar xzf -:
