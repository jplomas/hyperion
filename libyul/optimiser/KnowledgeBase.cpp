/*
	This file is part of hyperion.

	hyperion is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	hyperion is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with hyperion.  If not, see <http://www.gnu.org/licenses/>.
*/
// SPDX-License-Identifier: GPL-3.0
/**
 * Class that can answer questions about values of variables and their relations.
 */

#include <libyul/optimiser/KnowledgeBase.h>

#include <libyul/AST.h>
#include <libyul/Utilities.h>
#include <libyul/optimiser/DataFlowAnalyzer.h>

#include <libhyputil/CommonData.h>
#include <libhyputil/VMConstants.h>

#include <variant>

using namespace hyperion;
using namespace hyperion::yul;

KnowledgeBase::KnowledgeBase(std::map<YulString, AssignedValue> const& _ssaValues):
	m_valuesAreSSA(true),
	m_variableValues([_ssaValues](YulString _var) { return util::valueOrNullptr(_ssaValues, _var); })
{}

bool KnowledgeBase::knownToBeDifferent(YulString _a, YulString _b)
{
	if (std::optional<u512> difference = differenceIfKnownConstant(_a, _b))
		return difference != 0;
	return false;
}

std::optional<u512> KnowledgeBase::differenceIfKnownConstant(YulString _a, YulString _b)
{
	VariableOffset offA = explore(_a);
	VariableOffset offB = explore(_b);
	if (offA.reference == offB.reference)
		return offA.offset - offB.offset;
	else
		return std::nullopt;
}


bool KnowledgeBase::knownToBeDifferentByAtLeastWordSize(YulString _a, YulString _b)
{
	if (std::optional<u512> difference = differenceIfKnownConstant(_a, _b))
		return difference >= VMWordBytes && difference <= u512(0) - VMWordBytes;

	return false;
}

bool KnowledgeBase::knownToBeZero(YulString _a)
{
	return valueIfKnownConstant(_a) == 0;
}

std::optional<u512> KnowledgeBase::valueIfKnownConstant(YulString _a)
{
	return explore(_a).absoluteValue();
}

std::optional<u512> KnowledgeBase::valueIfKnownConstant(Expression const& _expression)
{
	if (Identifier const* ident = std::get_if<Identifier>(&_expression))
		return valueIfKnownConstant(ident->name);
	else if (Literal const* lit = std::get_if<Literal>(&_expression))
		return valueOfLiteral(*lit);
	else
		return std::nullopt;
}

void KnowledgeBase::variableChanged(YulString _var)
{
	yulAssert(!m_valuesAreSSA);
	yulAssert(!_var.empty());
	++m_currentVersions[_var];
}

KnowledgeBase::VersionedVariable KnowledgeBase::withCurrentVersion(YulString _var) const
{
	auto const version = m_currentVersions.find(_var);
	return VersionedVariable{_var, version == m_currentVersions.end() ? 0 : version->second};
}

KnowledgeBase::VariableOffset KnowledgeBase::explore(YulString _var)
{
	yulAssert(!_var.empty());
	VersionedVariable const currentVar = withCurrentVersion(_var);
	AssignedValue const* assignedValueWrapper = m_variableValues(_var);
	Expression const* assignedValue = assignedValueWrapper ? assignedValueWrapper->value : nullptr;
	// A versioned variable always refers to the same value,
	// so we check if we already computed it
	if (CachedOffset const* cachedOffset = util::valueOrNullptr(m_offsets, currentVar))
	{
		yulAssert(
			!assignedValue || cachedOffset->computedFrom == assignedValue,
			"Value changed without a call to variableChanged()."
		);
		return cachedOffset->offset;
	}

	VariableOffset offset{currentVar, 0};
	// This terminates only because the owner of the knowledge base is required to ensure that
	// a value does not reference _var, not even indirectly.
	// We explore the variables referenced by the value at their current version, as we cannot
	// tell which version was current when the value was assigned. This is only correct if none
	// of them changed since then. It is the owner's responsibility to return nullptr for the
	// value if they did.
	if (assignedValue)
		if (std::optional<VariableOffset> const offsetIfKnown = explore(*assignedValue))
			offset = *offsetIfKnown;
	m_offsets[currentVar] = CachedOffset{offset, assignedValue};
	return offset;
}

std::optional<KnowledgeBase::VariableOffset> KnowledgeBase::explore(Expression const& _value)
{
	if (Literal const* literal = std::get_if<Literal>(&_value))
		return VariableOffset{VersionedVariable{}, valueOfLiteral(*literal)};
	else if (Identifier const* identifier = std::get_if<Identifier>(&_value))
		return explore(identifier->name);
	else if (FunctionCall const* f = std::get_if<FunctionCall>(&_value))
	{
		if (f->functionName.name == "add"_yulstring)
		{
			if (std::optional<VariableOffset> a = explore(f->arguments[0]))
				if (std::optional<VariableOffset> b = explore(f->arguments[1]))
				{
					u512 offset = a->offset + b->offset;
					if (a->isAbsolute())
						// a is constant
						return VariableOffset{b->reference, offset};
					else if (b->isAbsolute())
						// b is constant
						return VariableOffset{a->reference, offset};
				}
		}
		else if (f->functionName.name == "sub"_yulstring)
			if (std::optional<VariableOffset> a = explore(f->arguments[0]))
				if (std::optional<VariableOffset> b = explore(f->arguments[1]))
				{
					u512 offset = a->offset - b->offset;
					if (a->reference == b->reference)
						return VariableOffset{VersionedVariable{}, offset};
					else if (b->isAbsolute())
						// b is constant
						return VariableOffset{a->reference, offset};
				}
	}

	return std::nullopt;
}
