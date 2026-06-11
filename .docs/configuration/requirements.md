This file specifies the options and configurations that the configuration files should be able to set.

## Global & Server Levels

| Configuration Parameter 	| Scope 			| Requirement Status 	| Default Value | Constraints |
|---						|---				|---					|---			|---|
| Website Definitions 		| Global 			| Required 				| N/A 			| Must define at least one website block. |
| Listening Interface 		| Server 			| Required 				| N/A 			| Must explicitly define both interface/host and port. A website can't have multiple listening interfaces. |
| Client Max Body Size 		| Route / Server	| Optional 				| 1 Megabyte 	| Inherited by routes. |
| Custom Error Pages 		| Server 			| Optional 				| N/A 			| User-defined custom pages for specific HTTP error codes. |

## Route / Location Levels (no regex matching required)

| Configuration Parameter 		| Scope | Required 	| Default Value | Constraints |
|---							|---	|---		|---			|---|
| Root Directory (root) 		| Route | Required 	| N/A 			| Must be defined for every location to map requests to the host filesystem. |
| Accepted HTTP Methods 		| Route | Optional 	| GET 			| Applies if no methods are explicitly listed. |
| HTTP Redirections				| Route | Optional 	| N/A 			| Configures URL forwarding to another destination. |
| Directory Listing (autoindex) | Route | Optional 	| N/A 			| Enables or disables the visibility of directory contents. |
| Default Index Files 			| Route | Optional 	| N/A 			| Specifies the fallback files to serve if the requested resource is a directory. |
| File Upload Storage 			| Route | Optional 	| N/A 			| Enables file uploads and defines the destination save path. |
| CGI Execution 				| Route | Optional 	| N/A 			| Triggers CGI script execution based on the requested file extension. |
