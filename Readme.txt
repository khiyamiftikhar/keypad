About

This is an interrupt based keypad library designed for ESPIDF. 
It can detect 'n' number of keys simulataneously where 'n' is set statically through Kconfig.
The detected buttons could be anywhere, whether same coulmn or row


Usage
      The main folder contains the usage example in keypad_test.c
        Callback working and keypad interface member not yet implemented

Working
    The callback provides the following event data
    even_type:
            0: Pressed
            1: Released    
            2: Long Pressed
            3: Repeat Press   //If kept pressed after 'Long Press Duration', this event will keep coming after every 'Repeat Press Duration'.

    key_val:
            The value assigned to the key using thee 'keymap' member of the config
             
    
