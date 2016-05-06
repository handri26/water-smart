/*
* IMPORTANT NOTE about TembooAccount.h
*
* TembooAccount.h contains your Temboo account information and must be included
* alongside main.c.
*/

#ifndef TEMBOOACCOUNT_H_
#define TEMBOOACCOUNT_H_

#define TEMBOO_ACCOUNT "yourtembooaccount"  // Your Temboo account name
#define TEMBOO_APP_KEY_NAME "yourtembooappname"  // Your Temboo app name
#define TEMBOO_APP_KEY "yourtembooappkey"  // Your Temboo app key

#endif /* TEMBOOACCOUNT_H_ */

/*
* The same TembooAccount.h file settings can be used for all of your Temboo programs.
* Keeping your account information in a separate file means you can share the
* main.c file without worrying that you forgot to delete your credentials.
*/
