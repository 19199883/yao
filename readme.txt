�����������������������ݺ���־���Ʒ������ϡ�


���ܣ�
tar -czvf - file | openssl des3 -salt -k password -out /path/to/file.tar.gz

���ܣ�
openssl des3 -d -k password -salt -in /path/to/file.tar.gz | tar xzf -:


���ӣ�
tar -czvf - -C ./ yao_download_night_data.sh | openssl des3 -salt -k 617999 -out ./yao_download_night_data.txt 