;	ftale.i - assembly language include files for faery tale adventure

		include		"structures.i"

	struct		shape
		word		abs_x			; coordinates
		word		abs_y
		word		rel_x
		word		rel_y
		byte		type 			; what number object is this
		byte		race			; race number
		byte		index 			; what image index
		byte		visible			; on-screen flag
		byte		weapon			; type of weapon carried
		byte		environ			; environment variable
		byte		goal			; current goal mode
		byte		tactic			; means to carry it out
		byte		state
		byte		facing			; current movement state and facing
		word		vitality		; also original object number
		byte		vel_x			; velocity variables
		byte		vel_y
;		ptr			source_struct	; address of generating structure */
	label		l_shape

		end

	struct 	fpage
		ptr		ri_page				; rasinfo structure for this page
		ptr		savecop				; copper list pointer
		long 	isv_x				
		long	isv_y
		word	obcount
		ptr		shape_queue
		ptr		backsave
		long	saveused
		word	witchx
		word	witchy
		word	witchdir
		word	wflag
	label	l_fpage

	struct seq_info
		word	width
		word	height
		word	count
		ptr		location
		ptr		maskloc
		word	bytes
		word	current_file
	label	l_seq_info

; sequences
		enum	0,PHIL,OBJECTS,ENEMY,RAFT,SETFIG,CARRIER,DRAGON

	struct object
		word		xc
		word		yc
		byte		ob_id
		byte		ob_stat
	label		l_object

