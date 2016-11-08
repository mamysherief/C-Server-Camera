#include <stdio.h>
#include <stdlib.h>
#include <crypt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <capture.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int main(int argc, char *argv[])
{
	int sockfd, newsockfd, portno, pid;
	int clilen;
	struct sockaddr_in serv_addr, cli_addr;
	media_frame  *frame;
	void     *data;
	char buffer[256];
	media_stream *stream;

	/*---- Create the socket----*/
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
	perror("ERROR opening socket");

	//syslog(LOG_INFO, “Socket is opened”);	

	/*---- Configure settings of the server address struct ----*/
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	/* Set port number, using htons function to use proper byte order */
	portno = 4444;
	serv_addr.sin_port = htons(portno);

	/*---- Bind the address struct to the socket ----*/
	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
	      perror("ERROR on binding");

	/*---- Listen on the socket, with 5 max connection requests queued ----*/
	listen(sockfd,5);
	clilen = sizeof(cli_addr);

	//syslog(LOG_INFO, “Waiting for connection ... ”);

	while (1) {
	/*---- Accept incoming connection ----*/
	 newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
	 if (newsockfd < 0)
	     perror("ERROR on accept");
	 pid = fork();
	 if (pid < 0)
	     perror("ERROR on fork");
	 if (pid == 0)  {
		// read buffer parameters that will set the resolution and fps
		//eg. capture-cameraIP=192.168.20.248&capture-userpass=root:passs&resolution=176x144&fps=1
		read(newsockfd,buffer,255);
		syslog(LOG_INFO, "the buffer is %s", buffer);
		stream = capture_open_stream(IMAGE_JPEG, buffer);
		close(sockfd);

		while (1) {

	     		frame  = capture_get_frame(stream);
	     		data   = capture_frame_data(frame);
	     		size_t size   = capture_frame_size(frame);

			//start sending frame size
			syslog(LOG_INFO, "capture frame_size %d", size);
			int frame_size = htonl((int)size);
			syslog(LOG_INFO, "size is %d, frame_size is %d",size, frame_size);
				//int x = 0;	
				//unsigned char image[frame_size];
			syslog(LOG_INFO, "trying to send frame size");
			write(newsockfd, (void*) &frame_size, sizeof(int)); //send frame_size to client
			syslog(LOG_INFO, "sent frame size");

			//start sending data (image)
	       		int row= 0;
	     		unsigned char rowData[size];

	     		for (row=0 ; row< size ; row++)
	     		{
	     			rowData[row]=((unsigned char*)data)[row];
	     		}
			//sending image data
	     		write(newsockfd, rowData, sizeof(rowData));
			syslog(LOG_INFO, "data has been sent");
	     		capture_frame_free(frame);
	     		
	     		}
		 	 capture_close_stream(stream);
		 	 exit(0);


	 }
	 else close(newsockfd);
	} /* end of while */

	return 0;
}
