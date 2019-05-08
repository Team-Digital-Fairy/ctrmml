#include <algorithm>
#include <stdexcept>

#include "player.h"
#include "input.h"
#include "song.h"
#include "track.h"
#include "stringf.h"

Basic_Player::Basic_Player(Song& song, Track& track)
	: song(&song),
	track(&track),
	enabled(true),
	position(0),
	loop_position(-1),
	loop_reset_position(-1),
	stack(),
	loop_count(0),
	loop_reset_count(0),
	max_stack_depth(10),
	play_time(0),
	on_time(0),
	off_time(0)
{
}

//! Throws an InputError.
void Basic_Player::error(const char* message) const
{
	throw InputError(reference, message);
}

void Basic_Player::stack_underflow(int type)
{
	if(type == Player_Stack::LOOP)
		error("unterminated '[]' loop");
	else if(type == Player_Stack::JUMP)
		error("unexpected ']' loop end");
	else if(type == Player_Stack::DRUM_MODE)
		error("drum routine contains no note");
	else
		error("unknown stack type (BUG, please report)");
}

//! Push to stack.
void Basic_Player::stack_push(const Player_Stack& frame)
{
	if(stack.size() >= max_stack_depth)
		error("stack overflow (depth limit reached)");
	stack_depth[frame.type]++;
	stack.push(frame);
}

//! Get the stack top, with type checking.
Player_Stack& Basic_Player::stack_top(Player_Stack::Type type)
{
	if(!stack.size())
		stack_underflow(type);
	Player_Stack& frame = stack.top();
	if(frame.type != type)
		stack_underflow(frame.type);
	return frame;
}

//! Pop the stack, with type checking.
Player_Stack Basic_Player::stack_pop(Player_Stack::Type type)
{
	Player_Stack frame = stack.top();
	stack.pop();
	if(frame.type != type)
		stack_underflow(frame.type);
	return frame;
}

//! Get the type of the top stack frame.
/*!
 * If the stack is empty, Player_Stack::MAX_STACK_TYPE is returned.
 */
Player_Stack::Type Basic_Player::get_stack_type()
{
	if(!stack.size())
		return Player_Stack::MAX_STACK_TYPE;
	return stack.top().type;
}

//! Get the stack depth for the specified type.
unsigned int Basic_Player::get_stack_depth(Player_Stack::Type type)
{
	return stack_depth[type];
}

//! Gets the timestamp of the last played event.
unsigned int Basic_Player::get_play_time() const
{
	return play_time;
}

//! Gets the current loop count.
unsigned int Basic_Player::get_loop_count() const
{
	return std::min(loop_reset_count, loop_count);
}

//! Resets the loop count.
void Basic_Player::reset_loop_count()
{
	loop_count = 0;
	loop_reset_count = 0;
	loop_reset_position = (position) ? position-1 : 0;
}

//! Gets the last parsed event.
const Event& Basic_Player::get_event() const
{
	return event;
}

//! Play one event.
/*!
 * This function first reads an event from the track. Then calls the abstract virtual function event_hook().
 * After that, loops and jump events are handled to set the next event position.
 */
void Basic_Player::step_event()
{
	// Set accumulated time
	play_time += on_time + off_time;
	on_time = 0;
	off_time = 0;
	if(position == loop_reset_position)
		loop_reset_count = loop_count;
	try
	{
		// Read the next event
		track_event = &track->get_event(position++);
		event = *track_event;
	}
	catch(std::out_of_range&)
	{
		// reached the end
		event = {Event::END, 0, 0, 0, reference};
		track_event = nullptr;
	}
	// Set new on/off time
	on_time = event.on_time;
	off_time = event.off_time;
	reference = event.reference;
	// Handle events
	switch(event.type)
	{
		case Event::LOOP_START:
			stack_push({Player_Stack::LOOP, track, position, 0, 0});
			break;
		case Event::LOOP_BREAK:
			// verify
			stack_top(Player_Stack::LOOP);
			// set param to end position to help with conversion
			track_event->param = stack.top().end_position;
			// Break if at the final loop iteration
			if(stack.top().loop_count == 1)
				position = stack_pop(Player_Stack::LOOP).end_position;
			break;
		case Event::LOOP_END:
			stack_top(Player_Stack::LOOP);
			stack.top().end_position = position;
			// Set loop count if zero
			if(stack.top().loop_count == 0)
				stack.top().loop_count = event.param;
			// Jump back
			if(--stack.top().loop_count > 0)
				position = stack.top().position;
			else
				stack_pop(Player_Stack::LOOP);
			break;
		case Event::SEGNO:
			loop_position = position;
			loop_reset_position = position;
			event_hook();
			break;
		case Event::JUMP:
			try
			{
				Track& new_track = song->get_track(event.param);
				// Push old position
				stack_push({Player_Stack::JUMP, track, position, 0, 0});
				// Set new position
				track = &new_track;
				position = 0;
			}
			catch(std::exception& ex)
			{
				error("jump destination doesn't exist");
			}
			break;
		case Event::END:
			if(stack.size())
			{
				// Pop old position
				track = stack_top(Player_Stack::JUMP).track;
				position = stack_pop(Player_Stack::JUMP).position;
			}
			else
			{
				if(loop_position != -1 && loop_hook())
				{
					position = loop_position;
					loop_count++;
				}
				else
				{
					enabled = false;
					// send a rest event here?
					end_hook();
				}
			}
			break;
		default:
			event_hook();
			break;
	}
}

//! Return false when playback is completed.
bool Basic_Player::is_enabled() const
{
	return enabled;
}

Player::Player(Song& song, Track& track, Player_Flag flag)
	: Basic_Player(song, track),
	flag(flag),
	note_count(0),
	rest_count(0),
	platform_state(),
	platform_update_mask(0),
	track_state(),
	track_update_mask(0)
{
}

#define FLAG(flag) (track_update_mask & (1<<(flag - Event::CHANNEL_CMD)))
#define FLAG_SET(flag) { track_update_mask |= (1<<(flag - Event::CHANNEL_CMD)); }
#define FLAG_CLR(flag) { track_update_mask &= ~(1<<(flag - Event::CHANNEL_CMD)); }
#define CH_STATE(var) track_state[var - Event::CHANNEL_CMD]
#define VOL_BIT 30 + Event::CHANNEL_CMD
#define BPM_BIT 31 + Event::CHANNEL_CMD
void Player::handle_drum_mode()
{
	if(get_stack_type() != Player_Stack::DRUM_MODE)
	{
		// First note event enters subroutine
		int offset = CH_STATE(Event::DRUM_MODE);
		try
		{
			Track& new_track = song->get_track(offset + event.param);
			// Push old position
			stack_push({Player_Stack::DRUM_MODE, track, position, (int)on_time, (int)off_time});
			// Set new position
			track = &new_track;
			position = 0;
			// Replace note
			on_time = 0;
			off_time = 0;
			event.type = Event::NOP;
		}
		catch(std::exception& ex)
		{
			error(stringf("drum mode error: track *%d is not defined (base %d, note %d)", event.param + offset, offset, event.param).c_str());
		}
	}
	else
	{
		// Second note exits the subroutine
		on_time = stack_top(Player_Stack::DRUM_MODE).end_position;
		off_time = stack_top(Player_Stack::DRUM_MODE).loop_count;
		position = stack_top(Player_Stack::DRUM_MODE).position;
		track = stack_pop(Player_Stack::DRUM_MODE).track;
	}
}
void Player::handle_event()
{
	switch(event.type)
	{
		case Event::NOTE:
			if(CH_STATE(Event::DRUM_MODE))
				handle_drum_mode();
			break;
		case Event::PLATFORM:
			try
			{
				const Tag& tag = song->get_platform_command(event.param);
				platform_update_mask |= parse_platform_event(tag, platform_state);
			}
			catch (std::out_of_range &)
			{
				error(stringf("Platform command %d is not defined", event.param).c_str());
			}
			break;
		case Event::TRANSPOSE_REL:
			CH_STATE(Event::TRANSPOSE) += event.param;
			FLAG_SET(Event::TRANSPOSE);
			break;
		case Event::VOL:
		case Event::VOL_REL:
			if(event.type != Event::VOL_REL)
				CH_STATE(Event::VOL_FINE) = 0;
			CH_STATE(Event::VOL_FINE) += event.param;
			FLAG_SET(Event::VOL_FINE);
			FLAG_SET(VOL_BIT);
			break;
		case Event::VOL_FINE_REL:
			CH_STATE(Event::VOL_FINE) += event.param;
			FLAG_SET(Event::VOL_FINE);
			FLAG_CLR(VOL_BIT);
			break;
		case Event::TEMPO_BPM:
			CH_STATE(Event::TEMPO) = event.param;
			FLAG_SET(Event::TEMPO);
			FLAG_SET(BPM_BIT);
			break;
		default:
			if(event.type >= Event::CHANNEL_CMD && event.type < Event::CMD_COUNT)
			{
				int type = event.type - Event::CHANNEL_CMD;
				track_state[type] = event.param;
				track_update_mask |= 1<<type;
				if(type == Event::VOL_FINE)
					FLAG_CLR(VOL_BIT);
				if(type == Event::TEMPO)
					FLAG_CLR(BPM_BIT);
			}
			break;
	}
}

//! Get the update flag for \p type.
bool Player::get_update_flag(Event::Type type) const
{
	if(type < Event::CHANNEL_CMD || type >= Event::CMD_COUNT)
		error("BUG: Unsupported event type");
	return FLAG(type);
}

//! Clear the update flag for \p type.
void Player::clear_update_flag(Event::Type type)
{
	if(type < Event::CHANNEL_CMD || type >= Event::CMD_COUNT)
		error("BUG: Unsupported event type");
	FLAG_CLR(type);
}

//! Get the update flag for the platform variable \p type.
bool Player::get_platform_flag(unsigned int type) const
{
	if(type > 31)
		return 0; // silently ignored
	return (platform_update_mask >> type) & 1;
}

//! Clear the update flag for the platform variable \p type.
void Player::clear_platform_flag(unsigned int type)
{
	if(type > 31)
		return;
	platform_update_mask &= ~(1 << type);
}

//! Return true if coarse volume.
bool Player::coarse_volume_flag() const
{
	return FLAG(VOL_BIT);
}

//! Return true if BPM set.
bool Player::bpm_flag() const
{
	return FLAG(BPM_BIT);
}

//! Get platform-specific variable.
int16_t Player::get_platform_var(int type) const
{
	if(type > 31)
		return 0;
	return platform_state[type];
}

//! Get event variable.
int16_t Player::get_var(Event::Type type) const
{
	if(type < Event::CHANNEL_CMD || type >= Event::CMD_COUNT)
		error("BUG: Unsupported event type");
	return CH_STATE(type);
}

void Player::event_hook()
{
	handle_event();
	if(~flag & PLAYER_SKIP)
		write_event();
}

bool Player::loop_hook()
{
	return 1;
}

void Player::end_hook()
{
	event = {Event::END, 0, 0, 0, reference};
	write_event();
}

//! Custom platform event parser.
/*!
 * Override functions should return a bitmask representing the event_state
 * variables that were modified by the functions.
 */
uint32_t Player::parse_platform_event(const Tag& tag, int16_t* platform_state)
{
	return 0;
}

//! Default write_event handler. This simply increments a note and rest counter.
void Player::write_event()
{
	// Handle NOTE and REST events here.
	if(event.type == Event::NOTE)
		note_count++;
	else if(event.type == Event::REST || event.type == Event::END)
		rest_count++;
}

//! Skip to a timestamp
void Player::skip_ticks(unsigned int ticks)
{
	if(!is_enabled())
	{
		play_time += ticks;
		return;
	}
	flag |= PLAYER_SKIP;
	while(ticks && is_enabled())
	{
		if(on_time > ticks)
		{
			on_time -= ticks;
			break;
		}
		play_time += on_time;
		ticks -= on_time;
		on_time = 0;

		if(off_time > ticks)
		{
			off_time -= ticks;
			break;
		}
		play_time += off_time;
		ticks -= off_time;
		off_time = 0;

		step_event();
	}
	play_time += ticks;
	flag &= ~PLAYER_SKIP;
}

//! Play track
void Player::play_tick()
{
	if(on_time)
	{
		on_time--;
		// key off
		if(!on_time && off_time)
		{
			event = {Event::REST, 0, 0, 0, reference};
			write_event();
		}
	}
	else if(off_time)
	{
		off_time--;
	}
	while(is_enabled() && !on_time && !off_time)
	{
		step_event();
	}
	play_time++;
}

//! Validates a track by playing it.
/*!
 * \exception InputError if any validation errors occur. These should be displayed to the user.
 */
Track_Validator::Track_Validator(Song& song, Track& track)
	: Basic_Player(song, track), segno_time(-1), loop_time(0)
{
	// step all the way to the end
	while(is_enabled())
		step_event();
}

void Track_Validator::event_hook()
{
	// Record timestamp of segno event.
	if(event.type == Event::SEGNO)
		segno_time = get_play_time();
}

bool Track_Validator::loop_hook()
{
	// do not loop
	return 0;
}

void Track_Validator::end_hook()
{
	if(segno_time >= 0)
		loop_time = get_play_time() - segno_time;
}

//! Gets the length of the loop section (i.e. from the position of the Event::SEGNO to the end of the track)
/*!
 * If there is no loop, 0 is returned.
 */
unsigned int Track_Validator::get_loop_length() const
{
	return loop_time;
}

//! Validates all tracks in a song.
/*!
 * \exception InputError if any validation errors occur. These should be displayed to the user.
 */
Song_Validator::Song_Validator(Song& song)
{
	for(auto it = song.get_track_map().begin(); it != song.get_track_map().end(); it++)
	{
		track_map.insert(std::make_pair(it->first, Track_Validator(song, it->second)));
	}
}

//! Gets a reference to the Track_Validator map.
/*!
 * The track validators contain the length and loop lengths of each track, which you can get with Player::get_play_time() and Track_Validator::get_loop_length() respectively.
 */
const std::map<uint16_t,Track_Validator>& Song_Validator::get_track_map() const
{
	return track_map;
}

