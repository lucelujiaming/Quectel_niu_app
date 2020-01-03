/*-----------------------------------------------------------------------------------------------*/
/**
  @file ql_virtual_at.h 
  @brief Virtual AT device  
*/
/*-----------------------------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------------------------------
  EDIT HISTORY
  This section contains comments describing changes made to the file.
  Notice that changes are listed in reverse chronological order.
  $Header: $
  when       who          what, where, why
  --------   ---          ----------------------------------------------------------
  20180908   tyler.kuang  Created .
-------------------------------------------------------------------------------------------------*/

#ifndef _QL_VIRTUAL_H_
#define _QL_VIRTUAL_H_


#define QL_VIRTUAL_AT_DEVICE "/dev/ql_poc_tty"


/*-----------------------------------------------------------------------------------------------*/
/**
  @brief Initialization function 
  @return  On success, the number of bytes written is returned, On error, -1 is returned
  */
/*-----------------------------------------------------------------------------------------------*/
int ql_at_init(void);


/*-----------------------------------------------------------------------------------------------*/
/**
  @brief writes up to count bytes from the buffer pointed buf to the AT device 
  @param[in] buf Buffer
  @param[in] len Length
  @return  On success, the number of bytes written is returned, On error, -1 is returned
  */
/*-----------------------------------------------------------------------------------------------*/
int ql_at_write(const void *buf, int len);

/*-----------------------------------------------------------------------------------------------*/
/**
  @brief Wapper of ql_at_write, formatted output conversion, max length is 256  
  @return  On success, the number of bytes written is returned, On error, -1 is returned
  */
/*-----------------------------------------------------------------------------------------------*/
int ql_at_print(const char *fmt, ...);

/*-----------------------------------------------------------------------------------------------*/
/**
  @brief Check wheather the AT device is ready for reading
  @return 
  0 - not ready
  1 - ready for reading 
  */
/*-----------------------------------------------------------------------------------------------*/
int ql_at_has_data(void);

/*-----------------------------------------------------------------------------------------------*/
/**
  @brief Attempts to read up to count bytes from AT device into the buffer starting at buf
  @param[in] buf
  @param[in] len
  @return  On success, the number of bytes read is returned, On error, -1 is returned
  */
/*-----------------------------------------------------------------------------------------------*/
int ql_at_read(void *buf, int len);

/*-----------------------------------------------------------------------------------------------*/
/**
  @brief Get file descriptor of tht AT device 
  @return The file descriptor 
  */
/*-----------------------------------------------------------------------------------------------*/
int ql_at_get_fd(void);


/*-----------------------------------------------------------------------------------------------*/
/**
  @brief Uninit function 
  @return Always 0 is returned 
  */
/*-----------------------------------------------------------------------------------------------*/
int ql_at_uinit(void);

#endif

