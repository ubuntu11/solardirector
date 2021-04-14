#ifndef PROJECT_H
#define PROJECT_H

/*define the debug level I want to see on debug*/
#define DEBUGLEV ( \
                   VERBOSE_MASTER | \
                   VERBOSE_LIBRARY | \
                   VERBOSE_ROUTER | \
                   VERBOSE_REPOSITORY | \
                   VERBOSE_PACKETS | \
                   VERBOSE_HWL | \
                   VERBOSE_PROTLAYER | \
                   VERBOSE_CHANNELLIST | \
                   VERBOSE_BUFMANAGEMENT | \
                   VERBOSE_SMADATALIB | \
                   VERBOSE_BCN | \
                   VERBOSE_PACKETS \
                  )
#endif
