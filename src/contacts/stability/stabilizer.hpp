// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef IPTSD_CONTACTS_STABILITY_STABILIZER_HPP
#define IPTSD_CONTACTS_STABILITY_STABILIZER_HPP

#include "../contact.hpp"
#include "config.hpp"

#include <common/casts.hpp>
#include <common/constants.hpp>
#include <common/types.hpp>

#include <algorithm>
#include <deque>
#include <iterator>
#include <type_traits>
#include <vector>

namespace iptsd::contacts::stability {

template <class T>
class Stabilizer {
public:
	static_assert(std::is_floating_point_v<T>);

private:
	Config<T> m_config;

	// The last n frames, with n being m_config.temporal_window.
	std::deque<std::vector<Contact<T>>> m_frames;

public:
	Stabilizer(Config<T> config)
		: m_config {config}
		, m_frames {std::max(config.temporal_window, casts::to<usize>(2))} {};

	/*!
	 * Resets the stabilizer by clearing the stored copies of the last frames.
	 */
	void reset()
	{
		for (auto &frame : m_frames)
			frame.clear();
	}

	/*!
	 * Stabilizes all contacts of a frame.
	 *
	 * @param[in,out] frame The list of contacts to stabilize.
	 */
	void stabilize(std::vector<Contact<T>> &frame)
	{
		// Stabilize contacts
		for (Contact<T> &contact : frame)
			this->stabilize_contact(contact, m_frames.back());

		auto nf = m_frames.front();

		// Clear the oldest stored frame
		m_frames.pop_front();
		nf.clear();

		// Copy the new frame
		std::copy(frame.begin(), frame.end(), std::back_inserter(nf));

		// Remove all contacts that are not temporally stable
		if (m_config.check_temporal_stability && m_config.temporal_window >= 2) {
			frame.clear();

			for (const Contact<T> &contact : nf) {
				if (!this->check_temporal(contact))
					continue;

				frame.push_back(contact);
			}
		}

		m_frames.push_back(nf);
	}

private:
	/*!
	 * Checks the temporal stability of a contact.
	 *
	 * A contact is temporally stable if it appears in all frames of the temporal window.
	 *
	 * @param[in] contact The contact to check.
	 * @return Whether the contact is present in all previous frames.
	 */
	[[nodiscard]] bool check_temporal(const Contact<T> &contact) const
	{
		// Contacts that can't be tracked are considered temporally stable.
		if (!contact.index.has_value())
			return true;

		const usize index = contact.index.value();

		// Iterate over the last frames and find the contact with the same index
		for (auto itr = m_frames.crbegin(); itr != m_frames.crend(); itr++) {
			const auto wrapper = Contact<T>::find_in_frame(index, *itr);

			if (!wrapper.has_value())
				return false;
		}

		return true;
	}

	/*!
	 * Stabilize a single contact.
	 *
	 * @param[in,out] contact The contact to stabilize.
	 * @param[in] frame The previous frame.
	 */
	void stabilize_contact(Contact<T> &contact, const std::vector<Contact<T>> &frame) const
	{
		// Contacts that can't be tracked can't be stabilized.
		if (!contact.index.has_value())
			return;

		if (m_config.temporal_window < 2)
			return;

		const usize index = contact.index.value();
		const auto wrapper = Contact<T>::find_in_frame(index, frame);

		if (!wrapper.has_value())
			return;

		const Contact<T> &last = wrapper.value();

		if (m_config.size_difference_threshold.has_value())
			this->stabilize_size(contact, last);

		if (m_config.movement_limits.has_value())
			this->stabilize_movement(contact, last);
	}

	/*!
	 * Stabilizes the size of the contact.
	 *
	 * @param[in,out] current The contact to stabilize.
	 * @param[in] last The contact to compare against.
	 */
	void stabilize_size(Contact<T> &current, const Contact<T> &last) const
	{
		if (!m_config.size_difference_threshold.has_value())
			return;

		const T size_thresh = m_config.size_difference_threshold.value();
		const Vector2<T> delta = (current.size - last.size).cwiseAbs();

		// Is the contact rapidly changing its size?
		if ((delta.array() <= size_thresh).all())
			return;

		current.size = last.size;
	}

	/*!
	 * Stabilizes the movement of the contact.
	 *
	 * @param[in,out] current The contact to stabilize.
	 * @param[in] last The contact to compare against.
	 */
	void stabilize_movement(Contact<T> &current, const Contact<T> &last) const
	{
		if (!m_config.movement_limits.has_value())
			return;

		const Vector2<T> limit = m_config.movement_limits.value();

		const Vector2<T> delta = current.mean - last.mean;
		const T distance = std::hypot(delta.x(), delta.y());

		// Is the contact moving too fast or too slow?
		if (distance >= limit.x() && distance <= limit.y()) {
			// Move in the direction, but just as much as necessary
			current.mean -= limit.x() * (delta / distance);
		} else {
			current.mean = last.mean;
		}
	}
};

} // namespace iptsd::contacts::stability

#endif // IPTSD_CONTACTS_STABILITY_STABILIZER_HPP
